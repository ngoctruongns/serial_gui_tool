#pragma once

#include <QObject>
#include <QSerialPort>

class SerialWorker : public QObject
{
    Q_OBJECT
public:
    explicit SerialWorker(QObject *parent = nullptr);
    ~SerialWorker();

    bool openPort(const QString &portName, int baudrate = 115200);
    void closePort();
    bool sendData(const QByteArray &data);
    bool isOpen() const { return serial_.isOpen(); }
    void clearBuffer();

signals:
    void dataReceived(const QByteArray &data);
    void portOpened();
    void portClosed();
    void errorOccurred(const QString &msg);

private slots:
    void handleReadyRead();

private:
    QSerialPort serial_;
};
