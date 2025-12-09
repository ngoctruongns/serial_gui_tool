#pragma once

#include <QMainWindow>
#include <QCheckBox>
#include <QLineEdit>
#include <QStringList>
#include <QColor>
#include "serial_worker.h"
#include "plot_widget.h"
#include "batch_command.h"

// Define file paths
#define COMMAND_FILE_PATH "cmd/command.txt"
#define BATCH_COMMAND_FILE_PATH "cmd/batch_command.txt"
#define QUICK_COMMAND_FILE_PATH "cmd/quick_command.txt"
#define QUICK_GROUP_FILE_PATH "cmd/quick_groups.txt"
#define SETTING_FILE_PATH "cmd/settings.txt"

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
    void setTextAndSendCommand(const QString &cmd);
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
    void sendAllCommands();

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
    void showMessageAutoClose(const QString &title, const QString &msg, int timeoutMs = 1500);
    void setupUi();

    QString loadFromFile(QString filePath);
    int saveToFile(QString fPath, const QString &content);

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
    QPushButton *spaceBtn_;
    QPushButton *openBtn_;
    QPushButton *closeBtn_;
    QPushButton *sendBtn_;
    QPushButton *cmdLoadBtn_;
    // Quick-send buttons: user-assignable shortcuts to send preset commands (10 buttons total)
    QPushButton *quickBtn1_;
    QPushButton *quickBtn2_;
    QPushButton *quickBtn3_;
    QPushButton *quickBtn4_;
    QPushButton *quickBtn5_;
    QPushButton *quickBtn6_;
    QPushButton *quickBtn7_;
    QPushButton *quickBtn8_;
    QPushButton *quickBtn9_;
    QPushButton *quickBtn10_;
    // Small inline editors next to each quick-send button so the user can
    // directly view and edit the associated command string.
    CommandLineEdit *quickEdit1_;
    CommandLineEdit *quickEdit2_;
    CommandLineEdit *quickEdit3_;
    CommandLineEdit *quickEdit4_;
    CommandLineEdit *quickEdit5_;
    CommandLineEdit *quickEdit6_;
    CommandLineEdit *quickEdit7_;
    CommandLineEdit *quickEdit8_;
    CommandLineEdit *quickEdit9_;
    CommandLineEdit *quickEdit10_;
    // Group labels for quick buttons
    QString quickGroup1Label_;
    QString quickGroup2Label_;
    QGroupBox *quickGroup1Box_;
    QGroupBox *quickGroup2Box_;

    // Batch command area: multi-line edit and Send All button placed under Group 2
    QPlainTextEdit *cmdListView_;
    QPushButton *sendAllBtn_;
    BatchProcessor *batchProc_;
    
    // Knob sub command GUI
    QPushButton *startBtn_;
    QPushButton *powerBtn_;
    QPushButton *ccwBtn_;
    QPushButton *cwBtn_;
    QPushButton *unlockBtn_;
    QPushButton *langBtn_;

    // Basic UI elements for serial port configuration and control
    QPushButton *searchUpBtn_;
    QPushButton *searchDownBtn_;
    QPushButton *clearBtn_;
    QCheckBox *hexCheck_;
    QCheckBox *sendHex_;
    QCheckBox *autoScrollCheck_;
    QCheckBox *logReadOnlyCheck_;
    QByteArray buffer_;
    QCompleter *completer_;
    QCompleter *commandCompleter_;
    QTimer *timer_;

    PlotWindow* plotWindow_ = nullptr;

    // Settings
    int logFontSize_ = 22;
    QString eolMode_ = "\n";  // "\n" for LF, "\r\n" for CR+LF
    bool autoSaveOnExit_ = false;  // Auto-save log when exiting app
    bool autoScrollEnabled_ = true;  // Auto-scroll to end of log
    // Colors for log view and search highlight
    QColor logBgColor_ = Qt::white;
    QColor logTextColor_ = Qt::black;
    QColor searchHighlightColor_ = Qt::yellow;

    // Settings management
    void openSettings();
    void saveSettings();
    void loadSettings();
    void saveQuickGroupLabels();
    void loadQuickGroupLabels();
    // Highlight rules UI
    void openHighlightRules();

private:
    // Highlighter for log view
    class LogHighlighter *highlighter_ = nullptr;
};
