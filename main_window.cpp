#include "main_window.h"
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

    // Build UI in separate function to keep constructor short
    setupUi();
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveFile);
    connect(clearLogsAction, &QAction::triggered, this, &MainWindow::clearLogs);
    connect(exitAction, &QAction::triggered, this, &MainWindow::exitApp);
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
    cmd.append('\n');

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
    QTextCursor cursor = logView_->textCursor();
    cursor.movePosition(QTextCursor::End);
    logView_->setTextCursor(cursor);
    logView_->ensureCursorVisible();
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
    fmt.setBackground(Qt::yellow);

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

void MainWindow::searchDown()
{
    QString term = searchLine_->text();
    if (term.isEmpty())
        return;

    // If this is the first search after pressing Enter, start from top
    if (term != lastSearchTerm_) {
        lastSearchTerm_ = term;
        logView_->moveCursor(QTextCursor::Start);
    }

    // Try to find next from current cursor; if not, wrap to start once
    if (!logView_->find(term, QTextDocument::FindCaseSensitively)) {
        logView_->moveCursor(QTextCursor::Start);
        if (!logView_->find(term, QTextDocument::FindCaseSensitively)) {
            showMessageAutoClose("Search", "Text not found!", 1500);
            return;
        }
    }
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
    } else {
        showMessageAutoClose("Search", "Text not found!", 1500);
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
            return;
        }
    }
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
        showMessageAutoClose("Info", "Log directory does not exist.", 2000);
        return;
    }

    // Get all .txt files in the log directory
    QStringList filters;
    filters << "*.txt";
    logDir.setNameFilters(filters);
    QFileInfoList files = logDir.entryInfoList();

    if (files.isEmpty()) {
        showMessageAutoClose("Info", "No log files to delete.", 2000);
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

    showMessageAutoClose("Info", QString("Deleted %1 log file(s).").arg(deletedCount), 2000);
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
