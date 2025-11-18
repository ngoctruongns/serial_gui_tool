#include "log_highlighter.h"
#include <QSettings>

LogHighlighter::LogHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    loadFromSettings();
}

void LogHighlighter::setRules(const QVector<HighlightRule> &rules)
{
    rules_ = rules;
    rehighlight();
}

void LogHighlighter::highlightBlock(const QString &text)
{
    if (rules_.isEmpty())
        return;

    // For each rule, find all occurrences in this block and set format
    for (const HighlightRule &r : rules_) {
        if (!r.enabled)
            continue;
        if (r.pattern.isEmpty())
            continue;

        QString pat = r.pattern;
        int patLen = pat.length();
        int start = 0;
        // Case-insensitive by default
        while (true) {
            int idx = text.indexOf(pat, start, Qt::CaseInsensitive);
            if (idx == -1)
                break;
            QTextCharFormat fmt;
            fmt.setForeground(Qt::black);
            fmt.setBackground(r.color);
            setFormat(idx, patLen, fmt);
            start = idx + std::max(1, patLen);
        }
    }
}

void LogHighlighter::loadFromSettings()
{
    QSettings s;
    s.beginGroup("HighlightRules");
    int count = s.value("Count", 0).toInt();
    QVector<HighlightRule> vec;
    for (int i = 0; i < count; ++i) {
        s.beginGroup(QString::number(i));
        HighlightRule r;
        r.pattern = s.value("pattern", "").toString();
        QString col = s.value("color", "#FFFF00").toString();
        r.color = QColor(col);
        r.enabled = s.value("enabled", true).toBool();
        s.endGroup();
        if (!r.pattern.isEmpty())
            vec.append(r);
    }
    s.endGroup();
    rules_ = vec;
    rehighlight();
}

void LogHighlighter::saveToSettings() const
{
    QSettings s;
    s.beginGroup("HighlightRules");
    s.remove(""); // clear previous
    s.setValue("Count", rules_.size());
    for (int i = 0; i < rules_.size(); ++i) {
        s.beginGroup(QString::number(i));
        s.setValue("pattern", rules_[i].pattern);
        s.setValue("color", rules_[i].color.name());
        s.setValue("enabled", rules_[i].enabled);
        s.endGroup();
    }
    s.endGroup();
}
