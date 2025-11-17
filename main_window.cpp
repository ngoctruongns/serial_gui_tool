#include "main_window.h"
#include "log_highlighter.h"
#include "highlight_rules_dialog.h"
#include <QApplication>
#include <QComboBox>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSerialPortInfo>
#include <QTextEdit>
#include <QTextStream>
#include <QVBoxLayout>
#include <QDialog>
#include <QPlainTextEdit>
#include <QKeyEvent>

// Implementation of CommandLineEdit with arrow key support
CommandLineEdit::CommandLineEdit(QWidget *parent)
    : QLineEdit(parent), historyIndex_(-1)
{
}

void CommandLineEdit::setCommandHistory(const QStringList &history)
{
    commandHistory_ = history;
    historyIndex_ = -1;  // Reset to no selection
}

void CommandLineEdit::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Up) {
        event->accept();
        if (commandHistory_.isEmpty())
            return;

        // Move to previous command
        if (historyIndex_ < commandHistory_.size() - 1) {
            historyIndex_++;
            setText(commandHistory_.at(commandHistory_.size() - 1 - historyIndex_));
        }
        return;
    }
    else if (event->key() == Qt::Key_Down) {
        event->accept();
        if (commandHistory_.isEmpty())
            return;

        // Move to next command (more recent)
        if (historyIndex_ > 0) {
            historyIndex_--;
            setText(commandHistory_.at(commandHistory_.size() - 1 - historyIndex_));
        } else if (historyIndex_ == 0) {
            historyIndex_ = -1;
            clear();
        }
        return;
    }

    // For any other key, reset history index when user types
    if (event->text().length() > 0 && !event->text()[0].isNull()) {
        historyIndex_ = -1;
    }

    QLineEdit::keyPressEvent(event);
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    // File menu: Open / Save / Exit
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QAction *openAction = new QAction(tr("Open"), this);
    QAction *saveAction = new QAction(tr("Save"), this);
    QAction *clearLogsAction = new QAction(tr("Clear Log Files"), this);
    QAction *exitAction = new QAction(tr("Exit"), this);
    // Standard shortcuts
    openAction->setShortcut(QKeySequence::Open);
    saveAction->setShortcut(QKeySequence::Save);
    clearLogsAction->setShortcut(QKeySequence("Ctrl+Shift+D"));
    exitAction->setShortcut(QKeySequence("Ctrl+Q"));
    fileMenu->addAction(openAction);
    fileMenu->addAction(saveAction);
    fileMenu->addAction(clearLogsAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    // View menu: Show / Close Plot
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    QAction *showPlotAction = new QAction(tr("Show Plot"), this);
    QAction *closePlotAction = new QAction(tr("Close Plot"), this);
    // Shortcuts for view actions
    showPlotAction->setShortcut(QKeySequence("Ctrl+Shift+P"));
    closePlotAction->setShortcut(QKeySequence("Ctrl+Shift+W"));
    viewMenu->addAction(showPlotAction);
    viewMenu->addAction(closePlotAction);

    // Settings menu
    QMenu *settingsMenu = menuBar()->addMenu(tr("&Settings"));
    QAction *settingsAction = new QAction(tr("Preferences"), this);
    settingsAction->setShortcut(QKeySequence("Ctrl+,"));
    settingsMenu->addAction(settingsAction);
    connect(settingsAction, &QAction::triggered, this, &MainWindow::openSettings);
    QAction *highlightSettingsAction = new QAction(tr("Highlight rules..."), this);
    // Shortcut for quick access to highlight settings
    highlightSettingsAction->setShortcut(QKeySequence("Ctrl+Shift+H"));
    highlightSettingsAction->setShortcutContext(Qt::ApplicationShortcut);
    settingsMenu->addAction(highlightSettingsAction);
    connect(highlightSettingsAction, &QAction::triggered, this, &MainWindow::openHighlightRules);

    // Build UI in separate function to keep constructor short
    setupUi();

    // Create highlighter bound to the log view's document
    highlighter_ = new LogHighlighter(logView_->document());

    // Load quick group labels and update the group boxes
    loadQuickGroupLabels();
    if (quickGroup1Box_)
        quickGroup1Box_->setTitle(quickGroup1Label_);
    if (quickGroup2Box_)
        quickGroup2Box_->setTitle(quickGroup2Label_);

    connect(saveAction, &QAction::triggered, this, &MainWindow::saveFile);
    connect(clearLogsAction, &QAction::triggered, this, &MainWindow::clearLogs);
    connect(exitAction, &QAction::triggered, this, &MainWindow::exitApp);

    // Load settings
    loadSettings();

    // Update auto-scroll checkbox based on loaded settings
    if (autoScrollCheck_) {
        autoScrollCheck_->setChecked(autoScrollEnabled_);
    }

    // Create worker AFTER UI is setup
    worker_ = new SerialWorker(this); // parent = this, no manual delete needed

    // Setup command completer from history
    updateCommandCompleter();
    connect(timer_, &QTimer::timeout, this, &MainWindow::timerHandler);

    // View menu: Show / Close Plot
    connect(showPlotAction, &QAction::triggered, this, &MainWindow::onShowPlotTriggered);
    connect(closePlotAction, &QAction::triggered, this, [this]() {
        if (plotWindow_)
            plotWindow_->close();
    });

    // Serial worker signals
    connect(worker_, &SerialWorker::dataReceived, this, &MainWindow::onDataReceived);
    connect(worker_, &SerialWorker::errorOccurred, this, &MainWindow::onError);

    closeBtn_->setEnabled(false);
    updatePortList();
    initFlag_ = true;
}

MainWindow::~MainWindow()
{
}

void MainWindow::updatePortList()
{
    portCombo_->clear();
    for (auto &info : QSerialPortInfo::availablePorts())
        portCombo_->addItem(info.portName());
}

void MainWindow::openSerial()
{
    if (!worker_) {
        QMessageBox::critical(this, "Error", "Serial worker not initialized");
        return;
    }

    QString port = portCombo_->currentText();
    if (port.isEmpty()) {
        QMessageBox::warning(this, "Warning", "No serial port selected!");
        return;
    }

    int baud = baudCombo_->currentData().toInt();
    if (worker_->openPort(port, baud)) {
        log("Opened " + port + " at " + QString::number(baud) + " baud.");
        openBtn_->setEnabled(false);
        closeBtn_->setEnabled(true);

        // Clear log, buffer and plot, serial buffer
        // clearLog();
        worker_->clearBuffer();
    }
}

void MainWindow::closeSerial()
{
    worker_->closePort();
    log("Closed port.");
    openBtn_->setEnabled(true);
    closeBtn_->setEnabled(false);
}

void MainWindow::sendCommand()
{
    if (!worker_ || !worker_->isOpen()) {
        QMessageBox::warning(this, "Warning", "Serial port not open");
        return;
    }

    QString cmd = commandLine_->text();
    if (cmd.isEmpty())
        return;

    // Add command to history if it's not empty
    addCommandToHistory(cmd);
    cmd.append(eolMode_);

    if (sendHex_->isChecked()) {
        // parse input string as hex
        QByteArray bytes;
        QStringList parts = cmd.split(' ', Qt::SkipEmptyParts);
        for (const QString &p : parts)
            bytes.append(static_cast<char>(p.toUInt(nullptr, 16)));
        worker_->sendData(bytes);
        log("TX (HEX): " + cmd);
    } else {
        worker_->sendData(cmd.toUtf8());
        log("TX: " + cmd);
    }
}

void MainWindow::onDataReceived(const QByteArray &data)
{
    if (initFlag_) {
        initFlag_ = false;
        return;
    }

    if (hexCheck_->isChecked()) {
        QString hex = data.toHex(' ').toUpper();
        log(hex);
    } else {
        QString strLine = QString::fromUtf8(data);
        log(strLine);
    }

    // Handle data string for
    buffer_.append(data);

    QByteArray line;
    int splitIndex = -1;

    // HEX MODE: tách theo 0x0A hoặc 0xDD
    while (true) {
        int idxLF = buffer_.indexOf(char(0x0A));  // '\n'
        int idxETX = buffer_.indexOf(char(0xDD)); // ETX
        if (idxLF == -1 && idxETX == -1)
            break;

        if (idxLF == -1)
            splitIndex = idxETX;
        else if (idxETX == -1)
            splitIndex = idxLF;
        else
            splitIndex = std::min(idxLF, idxETX);

        line = buffer_.left(splitIndex + 1);
        buffer_.remove(0, splitIndex + 1);

        if (hexCheck_->isChecked()) {
            // QString hex = line.toHex(' ').toUpper();
            // log("RX: " + hex);
        } else {
            QString strLine = QString::fromUtf8(line);
            if (line.endsWith('\n')) {
                onDataPlotter(strLine);
            }
        }
    }
}

void MainWindow::onDataPlotter(const QString &line)
{
    // Parse as Arduino type: "sensor1:23.5,sensor2:45.6,sensor3:78.9\n"
    QMap<QString, double> values;
    QStringList parts = line.split(',', Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        // qDebug() << part << "\n";
        // Check for key:value format
        if (part.count(':') != 1)
            continue;
        QStringList kv = part.split(':');
        if (kv.size() == 2) {
            QString key = kv[0];
            double val = kv[1].toDouble();
            values[key] = val;
        }
    }
    if (!values.isEmpty()) {
        emit newSerialData(values);
    }
}

void MainWindow::onError(const QString &msg)
{
    QMessageBox::critical(this, "Serial Error", msg);
}

void MainWindow::log(const QString &msg)
{
    logView_->insertPlainText(msg);

    // Auto scroll to end only if auto-scroll is enabled
    if (autoScrollEnabled_) {
        QTextCursor cursor = logView_->textCursor();
        cursor.movePosition(QTextCursor::End);
        logView_->setTextCursor(cursor);
        logView_->ensureCursorVisible();
    }

    // Update search highlights if search term is not empty
    if (!searchLine_->text().isEmpty()) {
        highlightSearchResults(searchLine_->text());
        updateSearchMatches(searchLine_->text());
        updateSearchCountLabel();
    }
}

void MainWindow::clearLog()
{
    // If there's content, save it into ./log with default filename before clearing
    QString contents = logView_->toPlainText();
    if (!contents.isEmpty()) {
        QDir d(QDir::currentPath());
        if (!d.exists("log")) {
            d.mkdir("log");
        }
        QString defaultName = QDateTime::currentDateTime().toString("yyMMdd_hhmmss");
        QString filePath = d.filePath(QString("log/log_%1.txt").arg(defaultName));
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << contents;
            file.close();
        }
    }

    logView_->clear();
    buffer_.clear();
    emit clearData();
    initFlag_ = true;

    // Reset search state
    searchMatches_.clear();
    currentSearchIndex_ = -1;
    lastSearchTerm_.clear();
    updateSearchCountLabel();
}

void MainWindow::openFile()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Open Log File"), QString(),
                                                tr("Text Files (*.txt);;All Files (*)"));
    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Open Failed"), tr("Unable to open file: %1").arg(path));
        return;
    }
    QTextStream in(&file);
    QString contents = in.readAll();
    file.close();

    logView_->setPlainText(contents);
    // refresh completer and highlights
    updateCompleter();
    highlightSearchResults(searchLine_->text());
    updateSearchMatches(searchLine_->text());
    currentSearchIndex_ = -1;
    updateSearchCountLabel();
}

void MainWindow::saveFile()
{
    // Suggest default filename: log_yymmdd_hhmmss.txt
    QString defaultName = QDateTime::currentDateTime().toString("yyMMdd_hhmmss");
    defaultName = QString("log_%1.txt").arg(defaultName);
    QString path = QFileDialog::getSaveFileName(this, tr("Save Log File"), defaultName,
                                                tr("Text Files (*.txt);;All Files (*)"));
    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Save Failed"), tr("Unable to save file: %1").arg(path));
        return;
    }
    QTextStream out(&file);
    out << logView_->toPlainText();
    file.close();
}

void MainWindow::exitApp()
{
    // Auto-save log if option is enabled and there's content
    if (autoSaveOnExit_) {
        QString contents = logView_->toPlainText();
        if (!contents.isEmpty()) {
            QDir d(QDir::currentPath());
            if (!d.exists("log")) {
                d.mkdir("log");
            }
            QString defaultName = QDateTime::currentDateTime().toString("yyMMdd_hhmmss");
            QString filePath = d.filePath(QString("log/log_%1.txt").arg(defaultName));
            QFile file(filePath);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                out << contents;
                file.close();
            }
        }
    }

    // Close plot window if open, then quit
    if (plotWindow_) {
        plotWindow_->close();
    }
    qApp->quit();
}

void MainWindow::searchLog()
{
    QString text = searchLine_->text();
    if (text.isEmpty())
        return;

    // Default behavior: find next occurrence from current cursor
    if (logView_->find(text, QTextDocument::FindCaseSensitively)) {
        logView_->setFocus();
    } else {
        // wrap-around: try from top once
        logView_->moveCursor(QTextCursor::Start);
        if (logView_->find(text, QTextDocument::FindCaseSensitively)) {
            logView_->setFocus();
        } else {
            showMessageAutoClose("Search", "Text not found!", 1500);
        }
    }
}

void MainWindow::updateCompleter()
{
    QString allText = logView_->toPlainText();
    QStringList words = allText.split(QRegExp("\\W+"), Qt::SkipEmptyParts);
    words.removeDuplicates();

    QCompleter *completer = new QCompleter(words, this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    searchLine_->setCompleter(completer);

    // Also update highlights for current search term
    highlightSearchResults(searchLine_->text());
}

void MainWindow::highlightSearchResults(const QString &term)
{
    // Use extra selections so highlights are not permanent edits to the document
    QList<QTextEdit::ExtraSelection> extras;
    if (term.isEmpty()) {
        logView_->setExtraSelections(extras);
        return;
    }

    QTextDocument *doc = logView_->document();
    QTextCursor cursor(doc);
    QTextCharFormat fmt;
    fmt.setBackground(searchHighlightColor_);
    // Keep text color consistent with log text color for readability
    fmt.setForeground(logTextColor_);

    while (true) {
        cursor = doc->find(term, cursor);
        if (cursor.isNull())
            break;
        QTextEdit::ExtraSelection sel;
        sel.cursor = cursor;
        sel.format = fmt;
        extras.append(sel);
    }
    logView_->setExtraSelections(extras);
}

void MainWindow::updateSearchMatches(const QString &term)
{
    // Find all matches and update the list
    searchMatches_.clear();
    currentSearchIndex_ = -1;

    if (term.isEmpty()) {
        updateSearchCountLabel();
        return;
    }

    QString allText = logView_->toPlainText();
    int pos = 0;
    while (true) {
        int idx = allText.indexOf(term, pos, Qt::CaseSensitive);
        if (idx == -1) break;
        searchMatches_.append(idx);
        pos = idx + term.length();
    }

    updateSearchCountLabel();
}

void MainWindow::updateSearchCountLabel()
{
    if (searchMatches_.isEmpty() || lastSearchTerm_.isEmpty()) {
        searchCountLabel_->setText("");
        return;
    }

    int total = searchMatches_.size();
    int current = (currentSearchIndex_ >= 0) ? (currentSearchIndex_ + 1) : 0;
    searchCountLabel_->setText(QString("%1/%2").arg(current).arg(total));
}

void MainWindow::searchDown()
{
    QString term = searchLine_->text();
    if (term.isEmpty())
        return;

    // If this is the first search after pressing Enter, start from top
    if (term != lastSearchTerm_) {
        lastSearchTerm_ = term;
        logView_->moveCursor(QTextCursor::Start);
        currentSearchIndex_ = -1;
    }

    // Try to find next from current cursor; if not, wrap to start once
    if (!logView_->find(term, QTextDocument::FindCaseSensitively)) {
        logView_->moveCursor(QTextCursor::Start);
        if (!logView_->find(term, QTextDocument::FindCaseSensitively)) {
            showMessageAutoClose("Search", "Text not found!", 1500);
            currentSearchIndex_ = -1;
            updateSearchCountLabel();
            return;
        }
    }

    // Update current index based on cursor position
    if (!searchMatches_.isEmpty()) {
        QTextCursor cursor = logView_->textCursor();
        int cursorPos = cursor.selectionStart();
        for (int i = 0; i < searchMatches_.size(); ++i) {
            if (searchMatches_[i] == cursorPos) {
                currentSearchIndex_ = i;
                break;
            }
        }
    }

    updateSearchCountLabel();
    logView_->setFocus();
}

void MainWindow::onSearchReturnPressed()
{
    QString term = searchLine_->text();
    if (term.isEmpty())
        return;

    // Just highlight all matches - don't jump to first occurrence
    // Highlight is done by updateCompleter which is already connected to textChanged
    highlightSearchResults(term);

    // Show message if no matches found
    QTextDocument *doc = logView_->document();
    QTextCursor cursor(doc);
    if (!doc->find(term, cursor).isNull()) {
        // Found at least one match - ready for Up/Down navigation
        lastSearchTerm_ = term;
        currentSearchIndex_ = 0;
        updateSearchCountLabel();
    } else {
        showMessageAutoClose("Search", "Text not found!", 1500);
        currentSearchIndex_ = -1;
        updateSearchCountLabel();
    }
}

void MainWindow::searchUp()
{
    QString term = searchLine_->text();
    if (term.isEmpty())
        return;

    // Try to find previous from current cursor; if not, wrap to end once
    if (!logView_->find(term, QTextDocument::FindBackward | QTextDocument::FindCaseSensitively)) {
        logView_->moveCursor(QTextCursor::End);
        if (!logView_->find(term,
                            QTextDocument::FindBackward | QTextDocument::FindCaseSensitively)) {
            showMessageAutoClose("Search", "Text not found!", 1500);
            currentSearchIndex_ = -1;
            updateSearchCountLabel();
            return;
        }
    }

    // Update current index based on cursor position
    if (!searchMatches_.isEmpty()) {
        QTextCursor cursor = logView_->textCursor();
        int cursorPos = cursor.selectionStart();
        for (int i = 0; i < searchMatches_.size(); ++i) {
            if (searchMatches_[i] == cursorPos) {
                currentSearchIndex_ = i;
                break;
            }
        }
    }

    updateSearchCountLabel();
    logView_->setFocus();
}

void MainWindow::timerHandler()
{
    // Periodic tasks can be handled here
}

void MainWindow::showMessageAutoClose(const QString &title, const QString &msg, int timeoutMs)
{
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setWindowTitle(title);
    msgBox->setText(msg);
    msgBox->setStandardButtons(QMessageBox::NoButton);
    msgBox->show();

    QTimer::singleShot(timeoutMs, msgBox, &QMessageBox::accept);
}

void MainWindow::clearLogs()
{
    QDir logDir(QDir::currentPath() + "/log");

    // Check if log directory exists
    if (!logDir.exists()) {
        showMessageAutoClose("Info", "Log directory does not exist.", 1500);
        return;
    }

    // Get all .txt files in the log directory
    QStringList filters;
    filters << "*.txt";
    logDir.setNameFilters(filters);
    QFileInfoList files = logDir.entryInfoList();

    if (files.isEmpty()) {
        showMessageAutoClose("Info", "No log files to delete.", 1500);
        return;
    }

    // Show confirmation dialog
    int count = files.count();
    QString message = QString("Delete %1 log file(s)?").arg(count);
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirm Delete", message,
                                                               QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;

    // Delete files
    int deletedCount = 0;
    for (const QFileInfo &fileInfo : files) {
        if (QFile::remove(fileInfo.absoluteFilePath())) {
            deletedCount++;
        }
    }

    showMessageAutoClose("Info", QString("Deleted %1 log file(s).").arg(deletedCount), 1500);
}

QString MainWindow::loadCommandsFromFile()
{
    QDir dir(QDir::currentPath());
    if (!dir.exists("cmd")) {
        dir.mkdir("cmd");
    }

    QString filePath = dir.filePath("cmd/command.txt");
    QFile file(filePath);

    if (!file.exists()) {
        return QString(); // Return empty string if file doesn't exist
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Unable to read command file: " + filePath);
        return QString();
    }

    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    return content;
}

void MainWindow::saveCommandsToFile(const QString &content)
{
    QDir dir(QDir::currentPath());
    if (!dir.exists("cmd")) {
        dir.mkdir("cmd");
    }

    QString filePath = dir.filePath("cmd/command.txt");
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Unable to write to command file: " + filePath);
        return;
    }

    QTextStream out(&file);
    out << content;
    file.close();
}

void MainWindow::loadCommands()
{
    // Create a dialog window for editing commands
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle(tr("Edit Commands"));
    dialog->setGeometry(100, 100, 600, 400);

    QVBoxLayout *layout = new QVBoxLayout(dialog);

    QLabel *label = new QLabel(tr("Enter commands (one per line):"));
    layout->addWidget(label);

    QPlainTextEdit *textEdit = new QPlainTextEdit(dialog);
    textEdit->setPlainText(loadCommandsFromFile());
    layout->addWidget(textEdit);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *saveBtn = new QPushButton(tr("Save"), dialog);
    QPushButton *cancelBtn = new QPushButton(tr("Cancel"), dialog);

    buttonLayout->addStretch();
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(cancelBtn);
    layout->addLayout(buttonLayout);

    // Connect buttons
    connect(saveBtn, &QPushButton::clicked, dialog, [this, textEdit, dialog]() {
        saveCommandsToFile(textEdit->toPlainText());
        showMessageAutoClose("Success", "Commands saved successfully.", 1500);
        dialog->accept();
    });

    connect(cancelBtn, &QPushButton::clicked, dialog, &QDialog::reject);

    dialog->exec();
}

void MainWindow::updateCommandCompleter()
{
    // Load commands from file
    QString content = loadCommandsFromFile();
    QStringList commands = content.split('\n', Qt::SkipEmptyParts);

    // Remove duplicates and empty strings
    commands.removeDuplicates();

    // Create and set completer for commandLine_
    commandCompleter_ = new QCompleter(commands, this);
    commandCompleter_->setCaseSensitivity(Qt::CaseInsensitive);
    commandCompleter_->setCompletionMode(QCompleter::PopupCompletion);
    commandLine_->setCompleter(commandCompleter_);

    // Also set history for arrow key navigation
    commandLine_->setCommandHistory(commands);
}

void MainWindow::addCommandToHistory(const QString &command)
{
    // Load existing commands
    QString content = loadCommandsFromFile();
    QStringList commands = content.split('\n', Qt::SkipEmptyParts);

    // Check if command already exists (case-insensitive)
    bool exists = false;
    for (const QString &cmd : commands) {
        if (cmd.compare(command, Qt::CaseInsensitive) == 0) {
            exists = true;
            break;
        }
    }

    // Add new command if it doesn't exist
    if (!exists) {
        commands.append(command);
        saveCommandsToFile(commands.join('\n'));

        // Update completer with new command
        updateCommandCompleter();
    }

    // Update history in commandLine_ (keep all commands including duplicates for browsing)
    // Reload commands for history browsing
    content = loadCommandsFromFile();
    QStringList historyList = content.split('\n', Qt::SkipEmptyParts);
    commandLine_->setCommandHistory(historyList);
}

void MainWindow::onShowPlotTriggered()
{
    if (!plotWindow_) {
        plotWindow_ = new PlotWindow(this);

        connect(this, &MainWindow::newSerialData, plotWindow_->findChild<PlotWidget *>(),
                &PlotWidget::updateData);
        connect(this, &MainWindow::clearData, plotWindow_->findChild<PlotWidget *>(),
                &PlotWidget::plotClear);
    }
    plotWindow_->show();
    plotWindow_->raise();
    plotWindow_->activateWindow();
}

void MainWindow::openSettings()
{
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle(tr("Settings"));
    dialog->setGeometry(100, 100, 500, 300);

    QVBoxLayout *layout = new QVBoxLayout(dialog);

    // Font Size Setting
    QHBoxLayout *fontLayout = new QHBoxLayout();
    QLabel *fontLabel = new QLabel(tr("Log View Font Size:"));
    QComboBox *fontCombo = new QComboBox();
    for (int size = 8; size <= 32; size += 2)
        fontCombo->addItem(QString::number(size), size);
    fontCombo->setCurrentText(QString::number(logFontSize_));
    fontLayout->addWidget(fontLabel);
    fontLayout->addWidget(fontCombo);
    fontLayout->addStretch();
    layout->addLayout(fontLayout);

    // EOL Mode Setting
    QHBoxLayout *eolLayout = new QHBoxLayout();
    QLabel *eolLabel = new QLabel(tr("End of Line (EOL):"));
    QComboBox *eolCombo = new QComboBox();
    eolCombo->addItem(tr("None"), "");
    eolCombo->addItem(tr("LF (\\n)"), "\n");
    eolCombo->addItem(tr("CR+LF (\\r\\n)"), "\r\n");
    // Set current selection
    int currentIndex = 1;  // Default to LF
    if (eolMode_ == "")
        currentIndex = 0;  // None
    else if (eolMode_ == "\r\n")
        currentIndex = 2;  // CR+LF
    eolCombo->setCurrentIndex(currentIndex);
    eolLayout->addWidget(eolLabel);
    eolLayout->addWidget(eolCombo);
    eolLayout->addStretch();
    layout->addLayout(eolLayout);

    // Quick Command Group 1 Label
    QHBoxLayout *group1Layout = new QHBoxLayout();
    QLabel *group1Label = new QLabel(tr("Group 1 Label:"));
    QLineEdit *group1Edit = new QLineEdit();
    group1Edit->setText(quickGroup1Label_);
    group1Layout->addWidget(group1Label);
    group1Layout->addWidget(group1Edit);
    layout->addLayout(group1Layout);

    // Quick Command Group 2 Label
    QHBoxLayout *group2Layout = new QHBoxLayout();
    QLabel *group2Label = new QLabel(tr("Group 2 Label:"));
    QLineEdit *group2Edit = new QLineEdit();
    group2Edit->setText(quickGroup2Label_);
    group2Layout->addWidget(group2Label);
    group2Layout->addWidget(group2Edit);
    layout->addLayout(group2Layout);

    // Log colors: background and text
    QHBoxLayout *bgColorLayout = new QHBoxLayout();
    QLabel *bgColorLabel = new QLabel(tr("Log background color:"));
    QPushButton *bgColorBtn = new QPushButton(dialog);
    bgColorBtn->setFixedWidth(60);
    bgColorBtn->setStyleSheet(QString("background-color: %1;").arg(logBgColor_.name()));
    bgColorLayout->addWidget(bgColorLabel);
    bgColorLayout->addWidget(bgColorBtn);
    bgColorLayout->addStretch();
    layout->addLayout(bgColorLayout);

    QHBoxLayout *textColorLayout = new QHBoxLayout();
    QLabel *textColorLabel = new QLabel(tr("Log text color:"));
    QPushButton *textColorBtn = new QPushButton(dialog);
    textColorBtn->setFixedWidth(60);
    textColorBtn->setStyleSheet(QString("background-color: %1;").arg(logTextColor_.name()));
    textColorLayout->addWidget(textColorLabel);
    textColorLayout->addWidget(textColorBtn);
    textColorLayout->addStretch();
    layout->addLayout(textColorLayout);

    // Search highlight color
    QHBoxLayout *searchHlLayout = new QHBoxLayout();
    QLabel *searchHlLabel = new QLabel(tr("Search highlight color:"));
    QPushButton *searchHlBtn = new QPushButton(dialog);
    searchHlBtn->setFixedWidth(60);
    searchHlBtn->setStyleSheet(QString("background-color: %1;").arg(searchHighlightColor_.name()));
    searchHlLayout->addWidget(searchHlLabel);
    searchHlLayout->addWidget(searchHlBtn);
    searchHlLayout->addStretch();
    layout->addLayout(searchHlLayout);

    // Auto Save on Exit Setting
    QHBoxLayout *autoSaveLayout = new QHBoxLayout();
    QCheckBox *autoSaveCheck = new QCheckBox(tr("Auto-save log when exiting"));
    autoSaveCheck->setChecked(autoSaveOnExit_);
    autoSaveCheck->setToolTip(tr("If checked, log will be automatically saved to log/ folder when exiting the app"));
    autoSaveLayout->addWidget(autoSaveCheck);
    autoSaveLayout->addStretch();
    layout->addLayout(autoSaveLayout);

    layout->addStretch();

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *okBtn = new QPushButton(tr("OK"));
    QPushButton *cancelBtn = new QPushButton(tr("Cancel"));
    buttonLayout->addStretch();
    buttonLayout->addWidget(okBtn);
    buttonLayout->addWidget(cancelBtn);
    layout->addLayout(buttonLayout);

    connect(okBtn, &QPushButton::clicked, dialog, [this, fontCombo, eolCombo, group1Edit, group2Edit, autoSaveCheck, dialog]() {
        logFontSize_ = fontCombo->currentData().toInt();
        eolMode_ = eolCombo->currentData().toString();
        quickGroup1Label_ = group1Edit->text();
        quickGroup2Label_ = group2Edit->text();
        autoSaveOnExit_ = autoSaveCheck->isChecked();

        // Apply font size to logView_
        // Apply font size and colors to logView_
        logView_->setStyleSheet(QString("font-size: %1px; background-color: %2; color: %3;")
                                    .arg(logFontSize_)
                                    .arg(logBgColor_.name())
                                    .arg(logTextColor_.name()));

        // Update group box titles
        if (quickGroup1Box_)
            quickGroup1Box_->setTitle(quickGroup1Label_);
        if (quickGroup2Box_)
            quickGroup2Box_->setTitle(quickGroup2Label_);

        saveSettings();
        saveQuickGroupLabels();
        dialog->accept();
    });

    // Color pickers: open QColorDialog and update button appearance and members
    connect(bgColorBtn, &QPushButton::clicked, this, [this, bgColorBtn]() {
        QColor c = QColorDialog::getColor(logBgColor_, this, tr("Choose log background color"));
        if (c.isValid()) {
            logBgColor_ = c;
            bgColorBtn->setStyleSheet(QString("background-color: %1;").arg(c.name()));
        }
    });
    connect(textColorBtn, &QPushButton::clicked, this, [this, textColorBtn]() {
        QColor c = QColorDialog::getColor(logTextColor_, this, tr("Choose log text color"));
        if (c.isValid()) {
            logTextColor_ = c;
            textColorBtn->setStyleSheet(QString("background-color: %1;").arg(c.name()));
        }
    });
    connect(searchHlBtn, &QPushButton::clicked, this, [this, searchHlBtn]() {
        QColor c = QColorDialog::getColor(searchHighlightColor_, this, tr("Choose search highlight color"));
        if (c.isValid()) {
            searchHighlightColor_ = c;
            searchHlBtn->setStyleSheet(QString("background-color: %1;").arg(c.name()));
        }
    });

    connect(cancelBtn, &QPushButton::clicked, dialog, &QDialog::reject);

    dialog->exec();
}

void MainWindow::openHighlightRules()
{
    HighlightRulesDialog dlg(this);
    // When dialog emits rulesChanged, update highlighter immediately
    connect(&dlg, &HighlightRulesDialog::rulesChanged, this, [this](const QVector<HighlightRule> &r) {
        if (highlighter_)
            highlighter_->setRules(r);
    });

    if (dlg.exec() == QDialog::Accepted) {
        QVector<HighlightRule> r = dlg.rules();
        if (highlighter_)
            highlighter_->setRules(r);
    }
}

void MainWindow::saveSettings()
{
    QDir dir(QDir::currentPath());
    if (!dir.exists("cmd"))
        dir.mkdir("cmd");

    QString filePath = dir.filePath("cmd/settings.txt");
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Unable to write to settings file: " + filePath);
        return;
    }

    QTextStream out(&file);
    out << "FontSize=" << logFontSize_ << "\n";

    QString eolModeStr;
    if (eolMode_ == "")
        eolModeStr = "NONE";
    else if (eolMode_ == "\r\n")
        eolModeStr = "CRLF";
    else
        eolModeStr = "LF";

    out << "EOLMode=" << eolModeStr << "\n";
    out << "AutoSaveOnExit=" << (autoSaveOnExit_ ? "true" : "false") << "\n";
    out << "AutoScroll=" << (autoScrollEnabled_ ? "true" : "false") << "\n";
    out << "LogBgColor=" << logBgColor_.name() << "\n";
    out << "LogTextColor=" << logTextColor_.name() << "\n";
    out << "SearchHighlightColor=" << searchHighlightColor_.name() << "\n";
    file.close();
}

void MainWindow::loadSettings()
{
    QDir dir(QDir::currentPath());
    if (!dir.exists("cmd"))
        return;

    QString filePath = dir.filePath("cmd/settings.txt");
    QFile file(filePath);

    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.isEmpty() || line.startsWith("#"))
            continue;

        QStringList parts = line.split("=");
        if (parts.size() != 2)
            continue;

        QString key = parts[0].trimmed();
        QString value = parts[1].trimmed();

        if (key == "FontSize") {
            logFontSize_ = value.toInt();
            if (logFontSize_ < 8)
                logFontSize_ = 22;
        } else if (key == "LogBgColor") {
            QColor c(value);
            if (c.isValid())
                logBgColor_ = c;
        } else if (key == "LogTextColor") {
            QColor c(value);
            if (c.isValid())
                logTextColor_ = c;
        } else if (key == "SearchHighlightColor") {
            QColor c(value);
            if (c.isValid())
                searchHighlightColor_ = c;
        } else if (key == "EOLMode") {
            if (value == "NONE")
                eolMode_ = "";
            else if (value == "CRLF")
                eolMode_ = "\r\n";
            else
                eolMode_ = "\n";
        } else if (key == "AutoSaveOnExit") {
            autoSaveOnExit_ = (value == "true");
        } else if (key == "AutoScroll") {
            autoScrollEnabled_ = (value == "true");
        }
    }
    file.close();
    // Apply colors and font size to log view
    logView_->setStyleSheet(QString("font-size: %1px; background-color: %2; color: %3;")
                                .arg(logFontSize_)
                                .arg(logBgColor_.name())
                                .arg(logTextColor_.name()));
}

void MainWindow::saveQuickGroupLabels()
{
    QDir dir(QDir::currentPath());
    if (!dir.exists("cmd"))
        dir.mkdir("cmd");

    QString filePath = dir.filePath("cmd/quick_groups.txt");
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    QTextStream out(&file);
    out << "Group1=" << quickGroup1Label_ << "\n";
    out << "Group2=" << quickGroup2Label_ << "\n";
    file.close();
}

void MainWindow::loadQuickGroupLabels()
{
    QDir dir(QDir::currentPath());
    if (!dir.exists("cmd"))
        return;

    QString filePath = dir.filePath("cmd/quick_groups.txt");
    QFile file(filePath);

    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.isEmpty() || line.startsWith("#"))
            continue;

        QStringList parts = line.split("=");
        if (parts.size() != 2)
            continue;

        QString key = parts[0].trimmed();
        QString value = parts[1].trimmed();

        if (key == "Group1") {
            quickGroup1Label_ = value;
        } else if (key == "Group2") {
            quickGroup2Label_ = value;
        }
    }
    file.close();
}
