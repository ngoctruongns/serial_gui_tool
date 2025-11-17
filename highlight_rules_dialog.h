#pragma once

#include <QDialog>
#include <QVector>
#include "log_highlighter.h"

class QLineEdit;
class QPushButton;
class QCheckBox;

class HighlightRulesDialog : public QDialog
{
    Q_OBJECT
public:
    explicit HighlightRulesDialog(QWidget *parent = nullptr);

    // Returns rules currently in the dialog
    QVector<HighlightRule> rules() const;

signals:
    void rulesChanged(const QVector<HighlightRule> &rules);

private slots:
    void chooseColor(int index);
    void accept() override;

private:
    static const int kMaxRules = 5; // show 5 quick slots
    QLineEdit *patternEdits[kMaxRules];
    QPushButton *colorButtons[kMaxRules];
    QCheckBox *enableChecks[kMaxRules];

    void loadFromSettings();
    void saveToSettings(const QVector<HighlightRule> &r);
};
