#pragma once

#include <QtCharts>
#include <QMap>

class PlotWidget : public QChartView {
    Q_OBJECT
public:
    explicit PlotWidget(QWidget *parent = nullptr);
    void plotClear();

public slots:
    void updateData(const QMap<QString, double> &values);

private:
    QChart *chart_;
    QValueAxis *axisX_;
    QValueAxis *axisY_;
    QMap<QString, QLineSeries*> seriesMap_;
    int currentX_ = 0;
};