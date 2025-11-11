#include "serial_worker.h"
#include <QDebug>

SerialWorker::SerialWorker(QObject *parent)
    : QObject(parent)
{
    connect(&serial_, &QSerialPort::readyRead, this, &SerialWorker::handleReadyRead);
}

SerialWorker::~SerialWorker()
{
    if (serial_.isOpen())
        serial_.close();
}

bool SerialWorker::openPort(const QString &portName, int baudrate)
{
    if (serial_.isOpen())
        serial_.close();

    serial_.setPortName(portName);
    serial_.setBaudRate(baudrate);
    serial_.setDataBits(QSerialPort::Data8);
    serial_.setParity(QSerialPort::NoParity);
    serial_.setStopBits(QSerialPort::OneStop);
    serial_.setFlowControl(QSerialPort::NoFlowControl);

    if (serial_.open(QIODevice::ReadWrite)) {
        emit portOpened();
        return true;
    } else {
        emit errorOccurred(serial_.errorString());
        return false;
    }
}

void SerialWorker::closePort()
{
    if (serial_.isOpen()) {
        serial_.close();
        emit portClosed();
    }
}

bool SerialWorker::sendData(const QByteArray &data)
{
    if (!serial_.isOpen())
        return false;
    return serial_.write(data) == data.size();
}

void SerialWorker::handleReadyRead()
{
    QByteArray data = serial_.readAll();
    emit dataReceived(data);
}

void SerialWorker::clearBuffer()
{
    if (serial_.isOpen()) {
        serial_.clear(QSerialPort::Input);
        serial_.clear(QSerialPort::Output);
    }
}