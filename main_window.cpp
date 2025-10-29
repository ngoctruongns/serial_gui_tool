#include "main_window.h"
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSerialPortInfo>
#include <QTextEdit>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    QWidget *central = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout(central);

    // Cấu hình GUI
    portCombo_ = new QComboBox(this);
    baudCombo_ = new QComboBox();
    openBtn_ = new QPushButton("Open");
    closeBtn_ = new QPushButton("Close");
    sendBtn_ = new QPushButton("Send");
    commandLine_ = new QLineEdit(this);
    logView_ = new QTextEdit(this);
    logView_->setReadOnly(true);
    hexCheck_ = new QCheckBox("HEX Mode");
    sendHex_ = new QCheckBox("Send HEX", this);
    sendHex_->setToolTip("Send data as hex instead of UTF-8");

    // Populate baud rates
    const QList<int> baudRates = {9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600};
    for (int b : baudRates)
        baudCombo_->addItem(QString::number(b), b);
    baudCombo_->setCurrentText("115200");

    QHBoxLayout *h1 = new QHBoxLayout();
    h1->setSpacing(5);
    h1->setContentsMargins(5, 5, 5, 5);

    QLabel *portLabel = new QLabel("Port:");
    h1->addWidget(portLabel);
    portLabel->setMinimumWidth(30);
    portLabel->setMaximumWidth(50);

    h1->addWidget(portCombo_);
    h1->addSpacing(20); // Add small space between port and baud

    QLabel *baudLabel = new QLabel("Baud:");
    h1->addWidget(baudLabel);
    baudLabel->setMinimumWidth(30);
    baudLabel->setMaximumWidth(50);

    h1->addWidget(baudCombo_);
    h1->addSpacing(10);
    h1->addWidget(hexCheck_);
    h1->addWidget(openBtn_);
    h1->addWidget(closeBtn_);

    QHBoxLayout *h2 = new QHBoxLayout();
    h2->setSpacing(4);
    h2->setContentsMargins(2, 2, 2, 2);

    h2->addWidget(commandLine_);
    h2->addWidget(sendHex_);
    h2->addWidget(sendBtn_);

    vbox->addLayout(h1);
    vbox->addLayout(h2);
    vbox->addWidget(logView_);

    setCentralWidget(central);
    setWindowTitle("Serial GUI Tool");

    // --- Create worker BEFORE any connect involving it ---
    worker_ = new SerialWorker(this); // parent = this, no manual delete needed

    // Kết nối tín hiệu
    connect(openBtn_, &QPushButton::clicked, this, &MainWindow::openSerial);
    connect(closeBtn_, &QPushButton::clicked, this, &MainWindow::closeSerial);
    connect(sendBtn_, &QPushButton::clicked, this, &MainWindow::sendCommand);

    connect(worker_, &SerialWorker::dataReceived, this, &MainWindow::onDataReceived);
    connect(worker_, &SerialWorker::errorOccurred, this, &MainWindow::onError);

    closeBtn_->setEnabled(false);
    updatePortList();
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
            QString hex = line.toHex(' ').toUpper();
            log("RX: " + hex);
        } else {
            log("RX: " + QString::fromUtf8(line));
        }
    }
}

void MainWindow::onError(const QString &msg)
{
    QMessageBox::critical(this, "Serial Error", msg);
}

void MainWindow::log(const QString &msg)
{
    logView_->append(msg);
}
