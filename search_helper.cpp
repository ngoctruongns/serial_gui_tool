#include "search_helper.h"

namespace SearchHelper {

QVector<QPair<int,int>> findAllMatches(const QString &text, const QString &term, Qt::CaseSensitivity cs)
{
    QVector<QPair<int,int>> out;
    if (term.isEmpty()) return out;
    int pos = 0;
    while (true) {
        int idx = text.indexOf(term, pos, cs);
        if (idx == -1) break;
        out.append(qMakePair(idx, term.length()));
        pos = idx + term.length();
    }
    return out;
}

int findNext(const QString &text, const QString &term, int from, Qt::CaseSensitivity cs)
{
    if (term.isEmpty()) return -1;
    if (from < 0) from = 0;
    int idx = text.indexOf(term, from, cs);
    return idx;
}

int findPrevious(const QString &text, const QString &term, int from, Qt::CaseSensitivity cs)
{
    if (term.isEmpty()) return -1;
    if (from <= 0) return -1;
    // search backwards: use lastIndexOf with an upper bound
    int idx = text.lastIndexOf(term, from - 1, cs);
    return idx;
}

} // namespace SearchHelper
