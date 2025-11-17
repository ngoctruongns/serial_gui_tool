#pragma once

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QVector>
#include <QColor>

struct HighlightRule {
    QString pattern;
    QColor color;
    bool enabled = true;
};

class LogHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit LogHighlighter(QTextDocument *parent = nullptr);

    void setRules(const QVector<HighlightRule> &rules);
    QVector<HighlightRule> rules() const { return rules_; }

    // Load / save rules from QSettings (group: "HighlightRules")
    void loadFromSettings();
    void saveToSettings() const;

protected:
    void highlightBlock(const QString &text) override;

private:
    QVector<HighlightRule> rules_;
};
