#include "main_window.h"

#include <QComboBox>
#include <QDateTime>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QGridLayout>
#include <QSerialPortInfo>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QCompleter>
#include <QCheckBox>
#include <QInputDialog>
#include <QDir>
#include <QTextStream>

// This file contains UI construction and widget wiring for MainWindow.
// Kept separate to make main_window.cpp shorter and focused on logic.

void MainWindow::setupUi()
{
    QWidget *central = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout(central);

    // Create UI elements
    portCombo_ = new QComboBox(this);
    baudCombo_ = new QComboBox();
    loadBtn_ = new QPushButton(tr("Find Port"));
    openBtn_ = new QPushButton(tr("Open"));
    spaceBtn_ = new QPushButton(tr("Space"));
    closeBtn_ = new QPushButton(tr("Close"));
    sendBtn_ = new QPushButton(tr("Send"));
    cmdLoadBtn_ = new QPushButton(tr("Load"));
    cmdLoadBtn_->setMaximumWidth(50);
    searchUpBtn_ = new QPushButton(tr("Up"));
    searchUpBtn_->setMaximumWidth(40);
    searchDownBtn_ = new QPushButton(tr("Dn"));
    searchDownBtn_->setMaximumWidth(40);
    clearBtn_ = new QPushButton(tr("Clear All"));
    commandLine_ = new CommandLineEdit(this);
    searchLine_ = new QLineEdit(this);
    searchCountLabel_ = new QLabel(this);
    searchCountLabel_->setText("");
    searchCountLabel_->setMaximumWidth(60);
    logView_ = new QPlainTextEdit(this);
    logView_->setReadOnly(true);
    logView_->setStyleSheet(QString("font-size: %1px;").arg(logFontSize_));
    // logView_->setStyleSheet("background-color: black; color: white;");
    // QFont font = logView_->font();
    // font.setPointSize(13);
    // logView_->setFont(font);
    hexCheck_ = new QCheckBox(tr("HEX"));
    sendHex_ = new QCheckBox(tr("HEX"), this);
    sendHex_->setToolTip(tr("Send data as hex instead of UTF-8"));
    autoScrollCheck_ = new QCheckBox(tr("Auto Scroll"), this);
    autoScrollCheck_->setChecked(true);
    autoScrollCheck_->setToolTip(tr("Automatically scroll to the end when new data arrives"));

    logReadOnlyCheck_ = new QCheckBox(tr("Read Only"), this);
    logReadOnlyCheck_->setChecked(true);
    logReadOnlyCheck_->setToolTip(tr("Set read only mode for log view"));

    // Quick-send buttons (user-assignable). They will be shown to the
    // right of the log view in two groups (0-4 and 5-9).
    quickBtn1_ = new QPushButton(tr("CMD0"), this);
    quickBtn2_ = new QPushButton(tr("CMD1"), this);
    quickBtn3_ = new QPushButton(tr("CMD2"), this);
    quickBtn4_ = new QPushButton(tr("CMD3"), this);
    quickBtn5_ = new QPushButton(tr("CMD4"), this);
    quickBtn6_ = new QPushButton(tr("CMD5"), this);
    quickBtn7_ = new QPushButton(tr("CMD6"), this);
    quickBtn8_ = new QPushButton(tr("CMD7"), this);
    quickBtn9_ = new QPushButton(tr("CMD8"), this);
    quickBtn10_ = new QPushButton(tr("CMD9"), this);
    // Default command properties are empty; user can set these in code or
    // (future) via a small settings dialog. We'll use the "command" Qt
    // property to store the command string for each button.
    for (int i = 1; i <= 10; ++i) {
        auto btn = (i == 1) ? quickBtn1_ : (i == 2) ? quickBtn2_ : (i == 3) ? quickBtn3_ :
                   (i == 4) ? quickBtn4_ : (i == 5) ? quickBtn5_ : (i == 6) ? quickBtn6_ :
                   (i == 7) ? quickBtn7_ : (i == 8) ? quickBtn8_ : (i == 9) ? quickBtn9_ : quickBtn10_;
        btn->setProperty("command", QString());
    }

    commandLine_->setPlaceholderText("Command input here...");
    searchLine_->setPlaceholderText("Search string input here...");

    // Load quick commands from cmd/quick_command.txt if present.
    {
        QDir dir(QDir::currentPath());
        if (!dir.exists("cmd"))
            dir.mkdir("cmd");
        QString filePath = dir.filePath("cmd/quick_command.txt");
        QFile file(filePath);
        if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QStringList lines;
            while (!in.atEnd()) {
                QString raw = in.readLine();
                QString t = raw.trimmed();
                // Skip comment lines that start with '#'
                if (t.startsWith('#'))
                    continue;
                // Preserve empty lines (they represent empty commands)
                lines.append(t);
            }
            file.close();
            // Assign loaded lines to quick buttons (up to 10)
            auto assignLine = [&](int idx, QPushButton *btn) {
                if (idx < lines.size()) {
                    QString s = lines.at(idx).trimmed();
                    btn->setProperty("command", s);
                    btn->setToolTip(s);
                }
            };
            assignLine(0, quickBtn1_);
            assignLine(1, quickBtn2_);
            assignLine(2, quickBtn3_);
            assignLine(3, quickBtn4_);
            assignLine(4, quickBtn5_);
            assignLine(5, quickBtn6_);
            assignLine(6, quickBtn7_);
            assignLine(7, quickBtn8_);
            assignLine(8, quickBtn9_);
            assignLine(9, quickBtn10_);
        }
    }

    QStringList historyList = {"Error", "RX:", "Variable_1"};
    completer_ = new QCompleter(historyList, this);
    completer_->setCaseSensitivity(Qt::CaseInsensitive);
    completer_->setCompletionMode(QCompleter::PopupCompletion);
    searchLine_->setCompleter(completer_);

    timer_ = new QTimer(this);
    timer_->setInterval(1000); // 1 second interval

    // Populate baud rates
    const QList<int> baudRates = {9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600};
    for (int b : baudRates)
        baudCombo_->addItem(QString::number(b), b);
    baudCombo_->setCurrentText("115200");

    QHBoxLayout *h1 = new QHBoxLayout();
    h1->setSpacing(5);
    h1->setContentsMargins(5, 5, 5, 5);

    QLabel *portLabel = new QLabel(tr("Port:"));
    h1->addWidget(portLabel);
    portLabel->setMinimumWidth(30);
    portLabel->setMaximumWidth(50);

    h1->addWidget(portCombo_);
    h1->addWidget(loadBtn_);
    h1->addSpacing(20); // Add small space between port and baud

    QLabel *baudLabel = new QLabel(tr("Baud:"));
    h1->addWidget(baudLabel);
    baudLabel->setMinimumWidth(30);
    baudLabel->setMaximumWidth(50);

    h1->addWidget(baudCombo_);
    h1->addSpacing(10);
    h1->addWidget(hexCheck_);
    h1->addWidget(autoScrollCheck_);
    h1->addWidget(logReadOnlyCheck_);
    h1->addWidget(spaceBtn_);
    h1->addWidget(openBtn_);
    h1->addWidget(closeBtn_);

    QGridLayout *g = new QGridLayout;
    g->addWidget(commandLine_, 0, 0);
    g->addWidget(sendHex_, 0, 1);

    // Put Load and Send buttons into a compact container
    QWidget *cmdContainer = new QWidget(this);
    QHBoxLayout *cmdLayout = new QHBoxLayout(cmdContainer);
    cmdLayout->setContentsMargins(0, 0, 0, 0);
    cmdLayout->setSpacing(2);
    cmdLayout->addWidget(cmdLoadBtn_);
    cmdLayout->addWidget(sendBtn_);
    g->addWidget(cmdContainer, 0, 2);

    // Put search controls into a compact layout: searchLine + Up/Down buttons + count label
    QWidget *searchContainer = new QWidget(this);
    QHBoxLayout *searchLayout = new QHBoxLayout(searchContainer);
    searchLayout->setContentsMargins(0, 0, 0, 0);
    searchLayout->setSpacing(2);
    searchLayout->addWidget(searchLine_, /*stretch=*/1);  // Let searchLine_ expand
    searchLayout->addWidget(searchUpBtn_, /*stretch=*/0);
    searchLayout->addWidget(searchDownBtn_, /*stretch=*/0);
    searchLayout->addWidget(searchCountLabel_, /*stretch=*/0);

    g->addWidget(searchContainer, 1, 0, 1, 2);  // Span across column 0 and 1
    g->addWidget(clearBtn_, 1, 2);

    vbox->addLayout(h1);
    vbox->addLayout(g);

    // Main area: log view on the left, quick-send buttons stacked vertically on the right
    QHBoxLayout *mainArea = new QHBoxLayout();
    mainArea->setContentsMargins(0, 0, 0, 0);
    mainArea->setSpacing(6);
    // Let logView_ expand to take most space
    mainArea->addWidget(logView_, /*stretch=*/1);

    QWidget *quickContainer = new QWidget(this);
    QVBoxLayout *quickLayout = new QVBoxLayout(quickContainer);
    quickLayout->setContentsMargins(5, 5, 5, 5);
    quickLayout->setSpacing(6);

    // Create two groups: Group 1 (buttons 0-4) and Group 2 (buttons 5-9)
    // Helper to create a group with border and label - buttons arranged vertically
    auto createQuickGroup = [this](const QString &title, QPushButton **btns, QPushButton **editBtns, int count) -> QGroupBox* {
        QGroupBox *groupBox = new QGroupBox(title, this);
        QVBoxLayout *groupLayout = new QVBoxLayout(groupBox);
        groupLayout->setContentsMargins(8, 8, 8, 8);
        groupLayout->setSpacing(4);

        for (int i = 0; i < count; ++i) {
            btns[i]->setMaximumWidth(80);
            btns[i]->setMinimumWidth(70);
            editBtns[i]->setMaximumWidth(32);
            editBtns[i]->setMinimumWidth(32);

            // Arrange button and edit button horizontally
            QWidget *row = new QWidget(this);
            QHBoxLayout *rowLayout = new QHBoxLayout(row);
            rowLayout->setContentsMargins(0, 0, 0, 0);
            rowLayout->setSpacing(2);
            rowLayout->addWidget(btns[i], 1);  // button gets more space
            rowLayout->addWidget(editBtns[i], 0);

            groupLayout->addWidget(row);
        }
        groupLayout->addStretch();
        return groupBox;
    };

    // Setup edit buttons
    quickEditBtn1_ = new QPushButton(tr("..."), this);
    quickEditBtn2_ = new QPushButton(tr("..."), this);
    quickEditBtn3_ = new QPushButton(tr("..."), this);
    quickEditBtn4_ = new QPushButton(tr("..."), this);
    quickEditBtn5_ = new QPushButton(tr("..."), this);
    quickEditBtn6_ = new QPushButton(tr("..."), this);
    quickEditBtn7_ = new QPushButton(tr("..."), this);
    quickEditBtn8_ = new QPushButton(tr("..."), this);
    quickEditBtn9_ = new QPushButton(tr("..."), this);
    quickEditBtn10_ = new QPushButton(tr("..."), this);

    // Create group 1 (buttons 0-4)
    QPushButton *group1Btns[] = {quickBtn1_, quickBtn2_, quickBtn3_, quickBtn4_, quickBtn5_};
    QPushButton *group1Edits[] = {quickEditBtn1_, quickEditBtn2_, quickEditBtn3_, quickEditBtn4_, quickEditBtn5_};
    quickGroup1Label_ = "Group 1";
    quickGroup1Box_ = createQuickGroup(quickGroup1Label_, group1Btns, group1Edits, 5);

    // Create group 2 (buttons 5-9)
    QPushButton *group2Btns[] = {quickBtn6_, quickBtn7_, quickBtn8_, quickBtn9_, quickBtn10_};
    QPushButton *group2Edits[] = {quickEditBtn6_, quickEditBtn7_, quickEditBtn8_, quickEditBtn9_, quickEditBtn10_};
    quickGroup2Label_ = "Group 2";
    quickGroup2Box_ = createQuickGroup(quickGroup2Label_, group2Btns, group2Edits, 5);

    quickLayout->addWidget(quickGroup1Box_);
    quickLayout->addWidget(quickGroup2Box_);
    // Batch command area: multi-line edit and Send All button placed under Group 2
    QLabel *batchLabel = new QLabel(tr("Batch Commands"), this);
    batchLabel->setAlignment(Qt::AlignLeft);
    cmdListView_ = new QPlainTextEdit(this);
    cmdListView_->setPlaceholderText(tr("Enter one command per line"));
    cmdListView_->setMaximumHeight(140);
    // Load previously saved batch commands (if any)
    cmdListView_->setPlainText(loadCommandsFromFile());
    sendAllBtn_ = new QPushButton(tr("Send All"), this);
    sendAllBtn_->setMaximumWidth(100);
    // Add label, editor and button to the quick panel
    quickLayout->addWidget(batchLabel);
    quickLayout->addWidget(cmdListView_);
    quickLayout->addWidget(sendAllBtn_);
    quickLayout->addStretch(1);
    mainArea->addWidget(quickContainer, /*stretch=*/0);

    vbox->addLayout(mainArea);

    setCentralWidget(central);
    setWindowTitle(tr("Serial GUI Tool"));

    // Connect UI signals
    connect(loadBtn_, &QPushButton::clicked, this, [this]() { updatePortList(); });
    connect(openBtn_, &QPushButton::clicked, this, &MainWindow::openSerial);
    connect(closeBtn_, &QPushButton::clicked, this, &MainWindow::closeSerial);
    connect(sendBtn_, &QPushButton::clicked, this, &MainWindow::sendCommand);
    connect(cmdLoadBtn_, &QPushButton::clicked, this, &MainWindow::loadCommands);
    connect(clearBtn_, &QPushButton::clicked, this, &MainWindow::clearLog);
    connect(searchUpBtn_, &QPushButton::clicked, this, &MainWindow::searchUp);
    connect(searchDownBtn_, &QPushButton::clicked, this, &MainWindow::searchDown);
    connect(autoScrollCheck_, &QCheckBox::stateChanged, this, [this](int state) {
        autoScrollEnabled_ = (state == Qt::Checked);
    });
    connect(logReadOnlyCheck_, &QCheckBox::stateChanged, this, [this](int state) {
        logView_->setReadOnly(state == Qt::Checked);
    });
    connect(spaceBtn_, &QPushButton::clicked, this, [this] () {
        this->log("======================================================\n\n\n");
    });
    connect(commandLine_, &QLineEdit::returnPressed, this, &MainWindow::sendCommand);
    connect(searchLine_, &QLineEdit::textChanged, this, &MainWindow::updateCompleter);
    connect(searchLine_, &QLineEdit::textChanged, this, &MainWindow::updateSearchMatches);
    // When user presses Enter in the search box: first Enter jumps to first match,
    // later Enters act like "search down" (next match)
    connect(searchLine_, &QLineEdit::returnPressed, this, &MainWindow::onSearchReturnPressed);
    connect(timer_, &QTimer::timeout, this, &MainWindow::timerHandler);

    // Quick button behavior: if a button has its "command" property set to a
    // non-empty string, set that into commandLine_ and send it when clicked.
    connect(quickBtn1_, &QPushButton::clicked, this, [this]() {
        QString cmd = quickBtn1_->property("command").toString();
        if (!cmd.isEmpty()) { commandLine_->setText(cmd); sendCommand(); }
    });
    connect(quickBtn2_, &QPushButton::clicked, this, [this]() {
        QString cmd = quickBtn2_->property("command").toString();
        if (!cmd.isEmpty()) { commandLine_->setText(cmd); sendCommand(); }
    });
    connect(quickBtn3_, &QPushButton::clicked, this, [this]() {
        QString cmd = quickBtn3_->property("command").toString();
        if (!cmd.isEmpty()) { commandLine_->setText(cmd); sendCommand(); }
    });
    connect(quickBtn4_, &QPushButton::clicked, this, [this]() {
        QString cmd = quickBtn4_->property("command").toString();
        if (!cmd.isEmpty()) { commandLine_->setText(cmd); sendCommand(); }
    });
    connect(quickBtn5_, &QPushButton::clicked, this, [this]() {
        QString cmd = quickBtn5_->property("command").toString();
        if (!cmd.isEmpty()) { commandLine_->setText(cmd); sendCommand(); }
    });
    connect(quickBtn6_, &QPushButton::clicked, this, [this]() {
        QString cmd = quickBtn6_->property("command").toString();
        if (!cmd.isEmpty()) { commandLine_->setText(cmd); sendCommand(); }
    });
    connect(quickBtn7_, &QPushButton::clicked, this, [this]() {
        QString cmd = quickBtn7_->property("command").toString();
        if (!cmd.isEmpty()) { commandLine_->setText(cmd); sendCommand(); }
    });
    connect(quickBtn8_, &QPushButton::clicked, this, [this]() {
        QString cmd = quickBtn8_->property("command").toString();
        if (!cmd.isEmpty()) { commandLine_->setText(cmd); sendCommand(); }
    });
    connect(quickBtn9_, &QPushButton::clicked, this, [this]() {
        QString cmd = quickBtn9_->property("command").toString();
        if (!cmd.isEmpty()) { commandLine_->setText(cmd); sendCommand(); }
    });
    connect(quickBtn10_, &QPushButton::clicked, this, [this]() {
        QString cmd = quickBtn10_->property("command").toString();
        if (!cmd.isEmpty()) { commandLine_->setText(cmd); sendCommand(); }
    });

    // Edit-button handlers: open an input dialog, store the string in the
    // quick button's "command" property and update its tooltip for visibility.
    // Helper to persist all quick commands to cmd/quick_command.txt
    auto saveAllQuickCommands = [this]() {
        QDir dir(QDir::currentPath());
        if (!dir.exists("cmd"))
            dir.mkdir("cmd");
        QString filePath = dir.filePath("cmd/quick_command.txt");
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            // If cannot open, silently ignore to avoid interrupting user
            return;
        }
        QTextStream out(&file);
        QStringList lines;
        lines << quickBtn1_->property("command").toString();
        lines << quickBtn2_->property("command").toString();
        lines << quickBtn3_->property("command").toString();
        lines << quickBtn4_->property("command").toString();
        lines << quickBtn5_->property("command").toString();
        lines << quickBtn6_->property("command").toString();
        lines << quickBtn7_->property("command").toString();
        lines << quickBtn8_->property("command").toString();
        lines << quickBtn9_->property("command").toString();
        lines << quickBtn10_->property("command").toString();
        out << lines.join('\n');
        file.close();
    };

    // Capture saveAllQuickCommands by value so the inner lambdas keep a valid
    // copy after setupUi() returns (capturing by reference caused a crash).
    auto makeEditHandler = [this, saveAllQuickCommands](QPushButton *quick, QPushButton *edit, const QString &label) {
        connect(edit, &QPushButton::clicked, this, [this, quick, label, saveAllQuickCommands]() {
            bool ok = false;
            QString current = quick->property("command").toString();
            QString text = QInputDialog::getText(this, tr("Set Quick Command"),
                                                tr("Command for %1:").arg(label),
                                                QLineEdit::Normal, current, &ok);
            if (ok) {
                quick->setProperty("command", text);
                quick->setToolTip(text);
                // Persist all quick commands to disk
                saveAllQuickCommands();
            }
        });
    };

    makeEditHandler(quickBtn1_, quickEditBtn1_, tr("CMD0"));
    makeEditHandler(quickBtn2_, quickEditBtn2_, tr("CMD1"));
    makeEditHandler(quickBtn3_, quickEditBtn3_, tr("CMD2"));
    makeEditHandler(quickBtn4_, quickEditBtn4_, tr("CMD3"));
    makeEditHandler(quickBtn5_, quickEditBtn5_, tr("CMD4"));
    makeEditHandler(quickBtn6_, quickEditBtn6_, tr("CMD5"));
    makeEditHandler(quickBtn7_, quickEditBtn7_, tr("CMD6"));
    makeEditHandler(quickBtn8_, quickEditBtn8_, tr("CMD7"));
    makeEditHandler(quickBtn9_, quickEditBtn9_, tr("CMD8"));
    makeEditHandler(quickBtn10_, quickEditBtn10_, tr("CMD9"));

    // Connect batch-send button
    connect(sendAllBtn_, &QPushButton::clicked, this, &MainWindow::sendAllCommands);

    // Keyboard shortcuts for search navigation: F3 = next, Shift+F3 = previous
    QShortcut *next = new QShortcut(QKeySequence("F3"), this);
    connect(next, &QShortcut::activated, this, &MainWindow::searchDown);
    QShortcut *prev = new QShortcut(QKeySequence("Shift+F3"), this);
    connect(prev, &QShortcut::activated, this, &MainWindow::searchUp);
}
