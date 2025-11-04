#pragma once

#include <QMainWindow>
#include <QCheckBox>
#include "serial_worker.h"
#include "plot_widget.h"

class QTextEdit;
class QPushButton;
class QComboBox;
class QLineEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void openSerial();
    void closeSerial();
    void sendCommand();
    void onDataReceived(const QByteArray &data);
    void onError(const QString &msg);

    private:
    void updatePortList();
    void log(const QString &msg);
    void onDataPlotter(const QString &line);
    void clearLog();
    // void searchInLog(const QString &searchStr);

    SerialWorker *worker_;
    PlotWidget *plotter_;
    QTextEdit *logView_;
    QComboBox *portCombo_;
    QComboBox *baudCombo_;
    QLineEdit *commandLine_;
    QLineEdit *searchLine_;
    QPushButton *loadBtn_;
    QPushButton *openBtn_;
    QPushButton *closeBtn_;
    QPushButton *sendBtn_;
    QPushButton *searchBtn_;
    QPushButton *clearBtn_;
    QCheckBox *hexCheck_;
    QCheckBox *sendHex_;
    QByteArray buffer_;
    bool initFlag_;
};
