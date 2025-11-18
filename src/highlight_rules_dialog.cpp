#include "highlight_rules_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QColorDialog>
#include <QSettings>

HighlightRulesDialog::HighlightRulesDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Highlight rules"));
    setModal(true);
    QVBoxLayout *v = new QVBoxLayout(this);

    for (int i = 0; i < kMaxRules; ++i) {
        QWidget *row = new QWidget(this);
        QHBoxLayout *h = new QHBoxLayout(row);
        h->setContentsMargins(0, 0, 0, 0);
        QLabel *lbl = new QLabel(tr("Pattern:"), this);
        patternEdits[i] = new QLineEdit(this);
        patternEdits[i]->setMaxLength(64);
        colorButtons[i] = new QPushButton(this);
        colorButtons[i]->setFixedWidth(36);
        enableChecks[i] = new QCheckBox(tr("Enable"), this);
        enableChecks[i]->setChecked(true);

        h->addWidget(lbl);
        h->addWidget(patternEdits[i], 1);
        h->addWidget(colorButtons[i]);
        h->addWidget(enableChecks[i]);
        v->addWidget(row);

        connect(colorButtons[i], &QPushButton::clicked, this, [this, i]() { chooseColor(i); });
    }

    QHBoxLayout *btns = new QHBoxLayout();
    btns->addStretch();
    QPushButton *ok = new QPushButton(tr("OK"), this);
    QPushButton *cancel = new QPushButton(tr("Cancel"), this);
    btns->addWidget(ok);
    btns->addWidget(cancel);
    v->addLayout(btns);

    connect(ok, &QPushButton::clicked, this, &HighlightRulesDialog::accept);
    connect(cancel, &QPushButton::clicked, this, &HighlightRulesDialog::reject);

    loadFromSettings();
}

QVector<HighlightRule> HighlightRulesDialog::rules() const
{
    QVector<HighlightRule> out;
    for (int i = 0; i < kMaxRules; ++i) {
        HighlightRule r;
        r.pattern = patternEdits[i]->text().trimmed();
        r.color = colorButtons[i]->palette().button().color();
        r.enabled = enableChecks[i]->isChecked();
        if (!r.pattern.isEmpty())
            out.append(r);
    }
    return out;
}

void HighlightRulesDialog::chooseColor(int index)
{
    QColor current = colorButtons[index]->palette().button().color();
    QColor c = QColorDialog::getColor(current, this, tr("Choose color"));
    if (!c.isValid())
        return;
    colorButtons[index]->setStyleSheet(QString("background-color: %1;").arg(c.name()));
}

void HighlightRulesDialog::accept()
{
    QVector<HighlightRule> r = rules();
    saveToSettings(r);
    emit rulesChanged(r);
    QDialog::accept();
}

void HighlightRulesDialog::loadFromSettings()
{
    QSettings s;
    s.beginGroup("HighlightRules");
    int count = s.value("Count", 0).toInt();
    for (int i = 0; i < kMaxRules; ++i) {
        if (i < count) {
            s.beginGroup(QString::number(i));
            QString pat = s.value("pattern", "").toString();
            QString col = s.value("color", "#FFFF00").toString();
            bool en = s.value("enabled", true).toBool();
            s.endGroup();
            patternEdits[i]->setText(pat);
            colorButtons[i]->setStyleSheet(QString("background-color: %1;").arg(col));
            enableChecks[i]->setChecked(en);
        } else {
            patternEdits[i]->clear();
            colorButtons[i]->setStyleSheet("background-color: #FFFF00;");
            enableChecks[i]->setChecked(false);
        }
    }
    s.endGroup();
}

void HighlightRulesDialog::saveToSettings(const QVector<HighlightRule> &r)
{
    QSettings s;
    s.beginGroup("HighlightRules");
    s.remove("");
    s.setValue("Count", r.size());
    for (int i = 0; i < r.size(); ++i) {
        s.beginGroup(QString::number(i));
        s.setValue("pattern", r[i].pattern);
        s.setValue("color", r[i].color.name());
        s.setValue("enabled", r[i].enabled);
        s.endGroup();
    }
    s.endGroup();
}
