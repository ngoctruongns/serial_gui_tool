#include "main_window.h"

#include <QComboBox>
#include <QDateTime>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QSplitter>
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
        QString filePath = dir.filePath(QUICK_COMMAND_FILE_PATH);
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

    // Main area: use a QSplitter so user can resize between log view and
    // the quick-send panel. This lets the command editors and batch area
    // expand when the user drags the splitter.
    QSplitter *split = new QSplitter(Qt::Horizontal, this);
    split->setContentsMargins(0, 0, 0, 0);

    // Let logView_ expand to take most space by default (splitter sizes set later)
    // Create quick panel container
    QWidget *quickContainer = new QWidget(this);
    QVBoxLayout *quickLayout = new QVBoxLayout(quickContainer);
    quickLayout->setContentsMargins(5, 5, 5, 5);
    quickLayout->setSpacing(6);

    // Create two groups: Group 1 (buttons 0-4) and Group 2 (buttons 5-9)
    // Helper to create a group with border and label - buttons arranged vertically
    // `editWidgets` can be any QWidget (we use `CommandLineEdit` instances)
    auto createQuickGroup = [this](const QString &title, QPushButton **btns, QWidget **editWidgets, int count) -> QGroupBox* {
        QGroupBox *groupBox = new QGroupBox(title, this);
        QVBoxLayout *groupLayout = new QVBoxLayout(groupBox);
        groupLayout->setContentsMargins(8, 8, 8, 8);
        groupLayout->setSpacing(4);

        for (int i = 0; i < count; ++i) {
            btns[i]->setMaximumWidth(80);
            btns[i]->setMinimumWidth(70);
            // Allow edit widgets to expand to fill remaining space; do not
            // cap the maximum width so they can grow to the splitter size.
            editWidgets[i]->setMinimumWidth(80);

            // Arrange button and edit button horizontally
            QWidget *row = new QWidget(this);
            QHBoxLayout *rowLayout = new QHBoxLayout(row);
            rowLayout->setContentsMargins(0, 0, 0, 0);
            rowLayout->setSpacing(2);
            // Make the quick button a fixed/preferred widget on the left
            btns[i]->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
            // Let the edit widget expand to take remaining horizontal space
            editWidgets[i]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            // Add button and editor, then add a trailing stretch so both
            // widgets are packed to the left and any extra space remains on the right.
            rowLayout->addWidget(btns[i], /*stretch=*/0);
            rowLayout->addWidget(editWidgets[i], /*stretch=*/1);
            rowLayout->addStretch(/*stretch=*/0);

            groupLayout->addWidget(row);
        }
        groupLayout->addStretch();
        return groupBox;
    };

    // Setup inline edit widgets (CommandLineEdit) so users can see/edit commands
    quickEdit1_ = new CommandLineEdit(this);
    quickEdit2_ = new CommandLineEdit(this);
    quickEdit3_ = new CommandLineEdit(this);
    quickEdit4_ = new CommandLineEdit(this);
    quickEdit5_ = new CommandLineEdit(this);
    quickEdit6_ = new CommandLineEdit(this);
    quickEdit7_ = new CommandLineEdit(this);
    quickEdit8_ = new CommandLineEdit(this);
    quickEdit9_ = new CommandLineEdit(this);
    quickEdit10_ = new CommandLineEdit(this);

    // Give a helpful placeholder so users know they can edit inline
    quickEdit1_->setPlaceholderText(tr("Edit command..."));
    quickEdit2_->setPlaceholderText(tr("Edit command..."));
    quickEdit3_->setPlaceholderText(tr("Edit command..."));
    quickEdit4_->setPlaceholderText(tr("Edit command..."));
    quickEdit5_->setPlaceholderText(tr("Edit command..."));
    quickEdit6_->setPlaceholderText(tr("Edit command..."));
    quickEdit7_->setPlaceholderText(tr("Edit command..."));
    quickEdit8_->setPlaceholderText(tr("Edit command..."));
    quickEdit9_->setPlaceholderText(tr("Edit command..."));
    quickEdit10_->setPlaceholderText(tr("Edit command..."));

    // Create group 1 (buttons 0-4)
    QPushButton *group1Btns[] = {quickBtn1_, quickBtn2_, quickBtn3_, quickBtn4_, quickBtn5_};
    QWidget *group1Edits[] = {quickEdit1_, quickEdit2_, quickEdit3_, quickEdit4_, quickEdit5_};
    quickGroup1Label_ = "Group 1";
    quickGroup1Box_ = createQuickGroup(quickGroup1Label_, group1Btns, group1Edits, 5);

    // Create group 2 (buttons 5-9)
    QPushButton *group2Btns[] = {quickBtn6_, quickBtn7_, quickBtn8_, quickBtn9_, quickBtn10_};
    QWidget *group2Edits[] = {quickEdit6_, quickEdit7_, quickEdit8_, quickEdit9_, quickEdit10_};
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
    cmdListView_->setPlainText(loadFromFile(BATCH_COMMAND_FILE_PATH));
    sendAllBtn_ = new QPushButton(tr("Send All"), this);
    sendAllBtn_->setMaximumWidth(100);
    batchProc_ = new BatchProcessor(this);

    // Add label, editor and button to the quick panel
    QGroupBox *batchGroupBox = new QGroupBox(tr("Batch Commands"), this);
    QVBoxLayout *batchLayout = new QVBoxLayout(batchGroupBox);
    batchLayout->setContentsMargins(8, 8, 8, 8);
    batchLayout->setSpacing(4);

    // Create a row for the Send All button to align it to the left
    QWidget *batchRow = new QWidget(this);
    QHBoxLayout *batchRowLayout = new QHBoxLayout(batchRow);
    batchRowLayout->setContentsMargins(0, 0, 0, 0);
    batchRowLayout->setSpacing(2);
    batchRowLayout->addWidget(sendAllBtn_, /*stretch=*/0);
    batchRowLayout->addStretch(/*stretch=*/1); // Push button to the left

    // Add label, text edit, and button row to the batch layout
    batchLayout->addWidget(cmdListView_);
    batchLayout->addWidget(batchRow);

    // Add the batch group box to the quick layout
    quickLayout->addWidget(batchGroupBox);
    quickLayout->addStretch(1); // Push everything to the top

    // Add widgets to the splitter and set reasonable initial sizes
    split->addWidget(logView_);
    split->addWidget(quickContainer);
    // Prefer log view to take most space initially
    split->setSizes({1000, 360});
    vbox->addWidget(split);

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

    // TODO: Use for loop with group1Btns, group2Btns to reduce LoC
    // Quick button behavior: if a button has its "command" property set to a
    // non-empty string, set that into commandLine_ and send it when clicked.
    connect(quickBtn1_, &QPushButton::clicked, this, [this]() {
        QString cmd = quickBtn1_->property("command").toString();
        setTextAndSendCommand(cmd);
    });
    connect(quickBtn2_, &QPushButton::clicked, this, [this]() {
        QString cmd = quickBtn2_->property("command").toString();
        setTextAndSendCommand(cmd);
    });
    connect(quickBtn3_, &QPushButton::clicked, this, [this]() {
        QString cmd = quickBtn3_->property("command").toString();
        setTextAndSendCommand(cmd);
    });
    connect(quickBtn4_, &QPushButton::clicked, this, [this]() {
        QString cmd = quickBtn4_->property("command").toString();
        setTextAndSendCommand(cmd);
    });
    connect(quickBtn5_, &QPushButton::clicked, this, [this]() {
        QString cmd = quickBtn5_->property("command").toString();
        setTextAndSendCommand(cmd);
    });
    connect(quickBtn6_, &QPushButton::clicked, this, [this]() {
        QString cmd = quickBtn6_->property("command").toString();
        setTextAndSendCommand(cmd);
    });
    connect(quickBtn7_, &QPushButton::clicked, this, [this]() {
        QString cmd = quickBtn7_->property("command").toString();
        setTextAndSendCommand(cmd);
    });
    connect(quickBtn8_, &QPushButton::clicked, this, [this]() {
        QString cmd = quickBtn8_->property("command").toString();
        setTextAndSendCommand(cmd);
    });
    connect(quickBtn9_, &QPushButton::clicked, this, [this]() {
        QString cmd = quickBtn9_->property("command").toString();
        setTextAndSendCommand(cmd);
    });
    connect(quickBtn10_, &QPushButton::clicked, this, [this]() {
        QString cmd = quickBtn10_->property("command").toString();
        setTextAndSendCommand(cmd);
    });

    // Edit handlers: when the inline editor finishes editing, store the
    // string in the quick button's "command" property and update its tooltip.
    // Helper to persist all quick commands to cmd/quick_command.txt
    auto saveAllQuickCommands = [this]() {
        QDir dir(QDir::currentPath());
        if (!dir.exists("cmd"))
            dir.mkdir("cmd");
        QString filePath = dir.filePath(QUICK_COMMAND_FILE_PATH);
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
    auto makeEditHandler = [this, saveAllQuickCommands](QPushButton *quick, CommandLineEdit *edit, const QString &label) {
        // When user finishes editing, update the quick command and persist
        connect(edit, &QLineEdit::editingFinished, this, [this, quick, edit, saveAllQuickCommands]() {
            QString text = edit->text().trimmed();
            quick->setProperty("command", text);
            quick->setToolTip(text);
            saveAllQuickCommands();
        });
    };

    makeEditHandler(quickBtn1_, quickEdit1_, tr("CMD0"));
    makeEditHandler(quickBtn2_, quickEdit2_, tr("CMD1"));
    makeEditHandler(quickBtn3_, quickEdit3_, tr("CMD2"));
    makeEditHandler(quickBtn4_, quickEdit4_, tr("CMD3"));
    makeEditHandler(quickBtn5_, quickEdit5_, tr("CMD4"));
    makeEditHandler(quickBtn6_, quickEdit6_, tr("CMD5"));
    makeEditHandler(quickBtn7_, quickEdit7_, tr("CMD6"));
    makeEditHandler(quickBtn8_, quickEdit8_, tr("CMD7"));
    makeEditHandler(quickBtn9_, quickEdit9_, tr("CMD8"));
    makeEditHandler(quickBtn10_, quickEdit10_, tr("CMD9"));

    // Populate inline editors from the quick button properties (loaded earlier)
    quickEdit1_->setText(quickBtn1_->property("command").toString());
    quickEdit1_->setToolTip(quickEdit1_->text().isEmpty() ? tr("Edit command...") : quickEdit1_->text());
    quickEdit2_->setText(quickBtn2_->property("command").toString());
    quickEdit2_->setToolTip(quickEdit2_->text().isEmpty() ? tr("Edit command...") : quickEdit2_->text());
    quickEdit3_->setText(quickBtn3_->property("command").toString());
    quickEdit3_->setToolTip(quickEdit3_->text().isEmpty() ? tr("Edit command...") : quickEdit3_->text());
    quickEdit4_->setText(quickBtn4_->property("command").toString());
    quickEdit4_->setToolTip(quickEdit4_->text().isEmpty() ? tr("Edit command...") : quickEdit4_->text());
    quickEdit5_->setText(quickBtn5_->property("command").toString());
    quickEdit5_->setToolTip(quickEdit5_->text().isEmpty() ? tr("Edit command...") : quickEdit5_->text());
    quickEdit6_->setText(quickBtn6_->property("command").toString());
    quickEdit6_->setToolTip(quickEdit6_->text().isEmpty() ? tr("Edit command...") : quickEdit6_->text());
    quickEdit7_->setText(quickBtn7_->property("command").toString());
    quickEdit7_->setToolTip(quickEdit7_->text().isEmpty() ? tr("Edit command...") : quickEdit7_->text());
    quickEdit8_->setText(quickBtn8_->property("command").toString());
    quickEdit8_->setToolTip(quickEdit8_->text().isEmpty() ? tr("Edit command...") : quickEdit8_->text());
    quickEdit9_->setText(quickBtn9_->property("command").toString());
    quickEdit9_->setToolTip(quickEdit9_->text().isEmpty() ? tr("Edit command...") : quickEdit9_->text());
    quickEdit10_->setText(quickBtn10_->property("command").toString());
    quickEdit10_->setToolTip(quickEdit10_->text().isEmpty() ? tr("Edit command...") : quickEdit10_->text());

    // Connect batch-send button
    connect(sendAllBtn_, &QPushButton::clicked, this, &MainWindow::sendAllCommands);
    connect(batchProc_, &BatchProcessor::sendBatchLineCmd, this, &MainWindow::setTextAndSendCommand);
    connect(batchProc_, &BatchProcessor::sendLog, this, &MainWindow::log);

    // Keyboard shortcuts for search navigation: F3 = next, Shift+F3 = previous
    QShortcut *next = new QShortcut(QKeySequence("F3"), this);
    connect(next, &QShortcut::activated, this, &MainWindow::searchDown);
    QShortcut *prev = new QShortcut(QKeySequence("Shift+F3"), this);
    connect(prev, &QShortcut::activated, this, &MainWindow::searchUp);
}
