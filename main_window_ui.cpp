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
    searchUpBtn_ = new QPushButton(tr("Up"));
    searchUpBtn_->setMaximumWidth(40);
    searchDownBtn_ = new QPushButton(tr("Dn"));
    searchDownBtn_->setMaximumWidth(40);
    clearBtn_ = new QPushButton(tr("Clear All"));
    commandLine_ = new QLineEdit(this);
    searchLine_ = new QLineEdit(this);
    logView_ = new QPlainTextEdit(this);
    logView_->setReadOnly(true);
    hexCheck_ = new QCheckBox(tr("HEX Mode"));
    sendHex_ = new QCheckBox(tr("Send HEX"), this);
    sendHex_->setToolTip(tr("Send data as hex instead of UTF-8"));

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
    g->addWidget(sendBtn_, 0, 2);
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
    vbox->addWidget(logView_);

    setCentralWidget(central);
    setWindowTitle(tr("Serial GUI Tool"));

    // Connect UI signals
    connect(loadBtn_, &QPushButton::clicked, this, [this]() { updatePortList(); });
    connect(openBtn_, &QPushButton::clicked, this, &MainWindow::openSerial);
    connect(closeBtn_, &QPushButton::clicked, this, &MainWindow::closeSerial);
    connect(sendBtn_, &QPushButton::clicked, this, &MainWindow::sendCommand);
    connect(clearBtn_, &QPushButton::clicked, this, &MainWindow::clearLog);
    connect(searchUpBtn_, &QPushButton::clicked, this, &MainWindow::searchUp);
    connect(searchDownBtn_, &QPushButton::clicked, this, &MainWindow::searchDown);
    connect(searchLine_, &QLineEdit::textChanged, this, &MainWindow::updateCompleter);
    connect(searchLine_, &QLineEdit::editingFinished, this, &MainWindow::searchLog);
    connect(timer_, &QTimer::timeout, this, &MainWindow::timerHandler);

    // Keyboard shortcuts for search navigation: F3 = next, Shift+F3 = previous
    QShortcut *next = new QShortcut(QKeySequence("F3"), this);
    connect(next, &QShortcut::activated, this, &MainWindow::searchDown);
    QShortcut *prev = new QShortcut(QKeySequence("Shift+F3"), this);
    connect(prev, &QShortcut::activated, this, &MainWindow::searchUp);
}
