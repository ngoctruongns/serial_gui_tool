#pragma once

#include <QtCharts>
#include <QMap>
#include <QMainWindow>

QT_CHARTS_USE_NAMESPACE

class PlotWidget : public QChartView {
    Q_OBJECT
public:
    explicit PlotWidget(QWidget *parent = nullptr);

public slots:
    void updateData(const QMap<QString, double> &values);
    void plotClear(void);

private:
    QChart *chart_;
    QValueAxis *axisX_;
    QValueAxis *axisY_;
    QMap<QString, QLineSeries*> seriesMap_;
    int currentX_ = 0;
};

// ----- PlotWindow  -----
class PlotWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit PlotWindow(QWidget *parent = nullptr);

private:
    PlotWidget *plotWidget_;
};