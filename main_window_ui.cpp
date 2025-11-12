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
    closeBtn_ = new QPushButton(tr("Close"));
    sendBtn_ = new QPushButton(tr("Send"));
    cmdLoadBtn_ = new QPushButton(tr("Load"));
    cmdLoadBtn_->setMaximumWidth(50);
    searchUpBtn_ = new QPushButton(tr("Up"));
    searchUpBtn_->setMaximumWidth(40);
    searchDownBtn_ = new QPushButton(tr("Dn"));
    searchDownBtn_->setMaximumWidth(40);
    clearBtn_ = new QPushButton(tr("Clear All"));
    commandLine_ = new QLineEdit(this);
    searchLine_ = new QLineEdit(this);
    logView_ = new QPlainTextEdit(this);
    logView_->setReadOnly(false);
    logView_->setStyleSheet("font-size: 22px;");
    // logView_->setStyleSheet("background-color: black; color: white;");
    // QFont font = logView_->font();
    // font.setPointSize(13);
    // logView_->setFont(font);
    hexCheck_ = new QCheckBox(tr("HEX"));
    sendHex_ = new QCheckBox(tr("HEX"), this);
    sendHex_->setToolTip(tr("Send data as hex instead of UTF-8"));

    // Quick-send buttons (user-assignable). They will be shown to the
    // right of the log view in a vertical arrangement.
    quickBtn1_ = new QPushButton(tr("Q1"), this);
    quickBtn2_ = new QPushButton(tr("Q2"), this);
    quickBtn3_ = new QPushButton(tr("Q3"), this);
    quickBtn4_ = new QPushButton(tr("Q4"), this);
    quickBtn5_ = new QPushButton(tr("Q5"), this);
    // Default command properties are empty; user can set these in code or
    // (future) via a small settings dialog. We'll use the "command" Qt
    // property to store the command string for each button.
    quickBtn1_->setProperty("command", QString());
    quickBtn2_->setProperty("command", QString());
    quickBtn3_->setProperty("command", QString());
    quickBtn4_->setProperty("command", QString());
    quickBtn5_->setProperty("command", QString());

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
            // Assign loaded lines to quick buttons (up to 5)
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

    g->addWidget(searchLine_, 1, 0);
    // Put the two small search navigation buttons into a compact container
    QWidget *searchNavContainer = new QWidget(this);
    QHBoxLayout *snLayout = new QHBoxLayout(searchNavContainer);
    snLayout->setContentsMargins(0, 0, 0, 0);
    snLayout->setSpacing(2);
    snLayout->addWidget(searchUpBtn_);
    snLayout->addWidget(searchDownBtn_);
    g->addWidget(searchNavContainer, 1, 1);
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
    quickLayout->setContentsMargins(0, 0, 0, 0);
    quickLayout->setSpacing(6);
    // Make buttons compact and vertically aligned from top to bottom
    quickBtn1_->setMaximumWidth(120);
    quickBtn2_->setMaximumWidth(120);
    quickBtn3_->setMaximumWidth(120);
    quickBtn4_->setMaximumWidth(120);
    quickBtn5_->setMaximumWidth(120);
    // For each quick button, create a small row containing the big quick
    // button and a small edit button to configure its command string.
    quickEditBtn1_ = new QPushButton(tr("..."), this);
    quickEditBtn2_ = new QPushButton(tr("..."), this);
    quickEditBtn3_ = new QPushButton(tr("..."), this);
    quickEditBtn4_ = new QPushButton(tr("..."), this);
    quickEditBtn5_ = new QPushButton(tr("..."), this);
    quickEditBtn1_->setMaximumWidth(36);
    quickEditBtn2_->setMaximumWidth(36);
    quickEditBtn3_->setMaximumWidth(36);
    quickEditBtn4_->setMaximumWidth(36);
    quickEditBtn5_->setMaximumWidth(36);

    auto addQuickRow = [&](QPushButton *quick, QPushButton *edit) {
        QWidget *row = new QWidget(this);
        QHBoxLayout *rl = new QHBoxLayout(row);
        rl->setContentsMargins(0, 0, 0, 0);
        rl->setSpacing(4);
        // Make quick button expand horizontally while edit stays compact
        rl->addWidget(quick);
        rl->addWidget(edit, /*stretch=*/0);
        quickLayout->addWidget(row);
    };

    addQuickRow(quickBtn1_, quickEditBtn1_);
    addQuickRow(quickBtn2_, quickEditBtn2_);
    addQuickRow(quickBtn3_, quickEditBtn3_);
    addQuickRow(quickBtn4_, quickEditBtn4_);
    addQuickRow(quickBtn5_, quickEditBtn5_);
    // Add a spacer to push buttons to the top
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
    connect(commandLine_, &QLineEdit::returnPressed, this, &MainWindow::sendCommand);
    connect(searchLine_, &QLineEdit::textChanged, this, &MainWindow::updateCompleter);
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

    makeEditHandler(quickBtn1_, quickEditBtn1_, tr("Q1"));
    makeEditHandler(quickBtn2_, quickEditBtn2_, tr("Q2"));
    makeEditHandler(quickBtn3_, quickEditBtn3_, tr("Q3"));
    makeEditHandler(quickBtn4_, quickEditBtn4_, tr("Q4"));
    makeEditHandler(quickBtn5_, quickEditBtn5_, tr("Q5"));

    // Keyboard shortcuts for search navigation: F3 = next, Shift+F3 = previous
    QShortcut *next = new QShortcut(QKeySequence("F3"), this);
    connect(next, &QShortcut::activated, this, &MainWindow::searchDown);
    QShortcut *prev = new QShortcut(QKeySequence("Shift+F3"), this);
    connect(prev, &QShortcut::activated, this, &MainWindow::searchUp);
}
