#pragma once

#include <QMainWindow>
#include <QCheckBox>
#include <QLineEdit>
#include <QStringList>
#include "serial_worker.h"
#include "plot_widget.h"

class QTextEdit;
class QPlainTextEdit;
class QCompleter;
class QTimer;
class QPushButton;
class QComboBox;
class QDialog;

// Custom QLineEdit with arrow key support for command history
class CommandLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    CommandLineEdit(QWidget *parent = nullptr);
    void setCommandHistory(const QStringList &history);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    QStringList commandHistory_;
    int historyIndex_;
};

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
    void onSearchReturnPressed();
    void openFile();
    void saveFile();
    void exitApp();
    void onShowPlotTriggered();
    void clearLogs();
    void loadCommands();

    private:
    void updatePortList();
    void log(const QString &msg);
    void onDataPlotter(const QString &line);
    void clearLog();
    void updateCompleter();
    void highlightSearchResults(const QString &term);
    void updateSearchMatches(const QString &term);
    void updateSearchCountLabel();
    void timerHandler();
    void showMessageAutoClose(const QString &title, const QString &msg, int timeoutMs = 2000);
    void setupUi();
    QString loadCommandsFromFile();
    void saveCommandsToFile(const QString &content);
    void updateCommandCompleter();
    void addCommandToHistory(const QString &command);

    // Track search state for Enter behavior in searchLine_
    QString lastSearchTerm_;
    bool searchReturnActive_ = false;
    int currentSearchIndex_ = -1;  // Current match index (0-based)
    QVector<int> searchMatches_;   // Positions of all matches

    bool initFlag_;
    SerialWorker *worker_;
    QPlainTextEdit *logView_;
    QComboBox *portCombo_;
    QComboBox *baudCombo_;
    CommandLineEdit *commandLine_;
    QLineEdit *searchLine_;
    QLabel *searchCountLabel_;     // Label to show "x/y" search count
    QPushButton *loadBtn_;
    QPushButton *openBtn_;
    QPushButton *closeBtn_;
    QPushButton *sendBtn_;
    QPushButton *cmdLoadBtn_;
    // Quick-send buttons: user-assignable shortcuts to send preset commands
    QPushButton *quickBtn1_;
    QPushButton *quickBtn2_;
    QPushButton *quickBtn3_;
    QPushButton *quickBtn4_;
    QPushButton *quickBtn5_;
    // Small edit buttons next to each quick-send button to configure the
    // command string shown/sent when the quick button is pressed.
    QPushButton *quickEditBtn1_;
    QPushButton *quickEditBtn2_;
    QPushButton *quickEditBtn3_;
    QPushButton *quickEditBtn4_;
    QPushButton *quickEditBtn5_;
    QPushButton *searchUpBtn_;
    QPushButton *searchDownBtn_;
    QPushButton *clearBtn_;
    QCheckBox *hexCheck_;
    QCheckBox *sendHex_;
    QByteArray buffer_;
    QCompleter *completer_;
    QCompleter *commandCompleter_;
    QTimer *timer_;

    PlotWindow* plotWindow_ = nullptr;

    // Settings
    int logFontSize_ = 22;
    QString eolMode_ = "\n";  // "\n" for LF, "\r\n" for CR+LF

    // Settings management
    void openSettings();
    void saveSettings();
    void loadSettings();
};
