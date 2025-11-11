#pragma once

#include <QString>
#include <QVector>
#include <QPair>
#include <QTextDocument>

namespace SearchHelper {
    // Return list of (start, length) for all matches of term inside text
    QVector<QPair<int,int>> findAllMatches(const QString &text, const QString &term, Qt::CaseSensitivity cs = Qt::CaseSensitive);

    // Find next occurrence index >= from (or -1 if not found)
    int findNext(const QString &text, const QString &term, int from, Qt::CaseSensitivity cs = Qt::CaseSensitive);

    // Find previous occurrence index < from (or -1 if not found)
    int findPrevious(const QString &text, const QString &term, int from, Qt::CaseSensitivity cs = Qt::CaseSensitive);
}
