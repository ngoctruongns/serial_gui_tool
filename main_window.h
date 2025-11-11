#pragma once

#include <QMainWindow>
#include <QCheckBox>
#include "serial_worker.h"
#include "plot_widget.h"

class QTextEdit;
class QPlainTextEdit;
class QCompleter;
class QTimer;
class QPushButton;
class QComboBox;
class QLineEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void newSerialData(const QMap<QString, double> &values);
    void clearData(void);

private slots:
    void openSerial();
    void closeSerial();
    void sendCommand();
    void onDataReceived(const QByteArray &data);
    void onError(const QString &msg);
    void searchLog();
    void searchUp();
    void searchDown();
    void openFile();
    void saveFile();
    void exitApp();
    void onShowPlotTriggered();
    void clearLogs();

    private:
    void updatePortList();
    void log(const QString &msg);
    void onDataPlotter(const QString &line);
    void clearLog();
    void updateCompleter();
    void highlightSearchResults(const QString &term);
    void timerHandler();
    void showMessageAutoClose(const QString &title, const QString &msg, int timeoutMs = 2000);
    void setupUi();

    bool initFlag_;
    SerialWorker *worker_;
    QPlainTextEdit *logView_;
    QComboBox *portCombo_;
    QComboBox *baudCombo_;
    QLineEdit *commandLine_;
    QLineEdit *searchLine_;
    QPushButton *loadBtn_;
    QPushButton *openBtn_;
    QPushButton *closeBtn_;
    QPushButton *sendBtn_;
    QPushButton *searchUpBtn_;
    QPushButton *searchDownBtn_;
    QPushButton *clearBtn_;
    QCheckBox *hexCheck_;
    QCheckBox *sendHex_;
    QByteArray buffer_;
    QCompleter *completer_;
    QTimer *timer_;

    PlotWindow* plotWindow_ = nullptr;
};
