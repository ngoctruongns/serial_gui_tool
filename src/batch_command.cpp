#include "batch_command.h"
#include <QTimer>
#include <QDebug>
#include <QRegularExpression>
#include <QRandomGenerator>


void BatchProcessor::processBatchFile(const QStringList &lines)
{
    commandLines = lines;
    currentIndex = 0;

    processNextCommand();
}

void BatchProcessor::processNextCommand(void)
{
    if (currentIndex >= commandLines.size()) {
        // emit finished();
        return;
    }

    QString raw = commandLines.at(currentIndex++);
    QString cmd = raw.trimmed();

    if (cmd.isEmpty()) {
        processNextCommand();
        return;
    }

    // Check special command
    static const QRegularExpression reDelaySec(R"(^\s*delay\(\s*([0-9]+(?:\.[0-9]+)?)\s*\)\s*$)",
                                                QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression reDelayMs(R"(^\s*delay_ms\(\s*([0-9]+)\s*\)\s*$)",
                                                QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression reRandDelay(R"(^\s*rand_delay\(\s*([0-9]+)\s*,\s*([0-9]+)\s*\)\s*$)",
                                                QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression reComment(R"(^\s*comment\(.*\)\s*$)",
                                                QRegularExpression::CaseInsensitiveOption);

    // Check for comment lines (start with # or comment(...))
    if (cmd.startsWith('#') || reComment.match(cmd).hasMatch()) {
        processNextCommand();
        return;
    }

    // Check for "delay(seconds)" special command
    QRegularExpressionMatch m = reDelaySec.match(cmd);
    if (m.hasMatch()) {
        double secs = m.captured(1).toDouble();
        unsigned long ms = static_cast<unsigned long>(secs * 1000.0);
        if (ms > 0) {
            unsigned long ms = static_cast<unsigned long>(secs * 1000.0);
            // Using QTimer::singleShot  to non block UI
            emit sendLog(QString(">>: Delay %1 seconds ...\n").arg(secs));
            QTimer::singleShot(ms, this, &BatchProcessor::processNextCommand);
        }
        return;
    }

    // Check for "delay_ms(ms)" special command
    m = reDelayMs.match(cmd);
    if (m.hasMatch()) {
        unsigned long ms = m.captured(1).toULong();
        if (ms > 0) {
            emit sendLog(QString(">>: Delay %1 ms ...\n").arg(ms));
            QTimer::singleShot(ms, this, &BatchProcessor::processNextCommand);
        }
        return;
    }

    // Check for "rand_delay(min_ms, max_ms)" special command
    m = reRandDelay.match(cmd);
    if (m.hasMatch()) {
        quint32 minMs = m.captured(1).toUInt();
        quint32 maxMs = m.captured(2).toUInt();
        if (minMs > maxMs)
            std::swap(minMs, maxMs);
        quint32 delayMs = QRandomGenerator::global()->bounded(minMs, maxMs + 1);
        emit sendLog(QString(">>: Random Delay %1 ms (range %2-%3 ms)\n").arg(delayMs).arg(minMs).arg(maxMs));
        if (delayMs > 0) {
            QTimer::singleShot(delayMs, this, &BatchProcessor::processNextCommand);
        }
        return;
    }

    // Send normal command
    emit sendBatchLineCmd(cmd);
    processNextCommand();
}
