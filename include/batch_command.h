#pragma once

#include <QObject>
#include <QStringList>

class BatchProcessor : public QObject
{
    Q_OBJECT

public:
    explicit BatchProcessor(QObject *parent = nullptr) : QObject(parent) {}

    void processBatchFile(const QStringList &lines);

signals:
    void sendBatchLineCmd(const QString &cmd);
    void sendLog(const QString &msg);
    // void finished();

private slots:
    void processNextCommand();

private:
    QStringList commandLines;
    int currentIndex;
};