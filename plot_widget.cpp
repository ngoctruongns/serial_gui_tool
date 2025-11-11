#include "plot_widget.h"

using namespace QtCharts;

PlotWidget::PlotWidget(QWidget *parent)
    : QChartView(parent), chart_(new QChart()), axisX_(new QValueAxis()), axisY_(new QValueAxis())
{
    setChart(chart_);
    chart_->addAxis(axisX_, Qt::AlignBottom);
    chart_->addAxis(axisY_, Qt::AlignLeft);
    axisX_->setTitleText("X Axis");
    axisY_->setTitleText("Y Axis");
    axisX_->setRange(0, 100);
    axisY_->setRange(0, 100);
}

void PlotWidget::updateData(const QMap<QString, double> &values)
{
    for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
        const QString &key = it.key();
        double value = it.value();

        if (!seriesMap_.contains(key)) {
            QLineSeries *series = new QLineSeries();
            series->setName(key);
            chart_->addSeries(series);
            series->attachAxis(axisX_);
            series->attachAxis(axisY_);
            seriesMap_[key] = series;
        }

        QLineSeries *series = seriesMap_[key];
        series->append(currentX_, value);
    }
    currentX_++;
    axisX_->setRange(qMax(0, currentX_ - 100), currentX_);
}

void PlotWidget::plotClear()
{
    for (auto series : seriesMap_) {
        chart_->removeSeries(series);
        delete series;
    }
    seriesMap_.clear();
    currentX_ = 0;
    axisX_->setRange(0, 100);
    axisY_->setRange(0, 100);
}

// ========== PlotWindow ========== //

PlotWindow::PlotWindow(QWidget *parent)
    : QMainWindow(parent),
      plotWidget_(new PlotWidget(this))
{
    setCentralWidget(plotWidget_);
    setWindowTitle("Plot Viewer");
    resize(700, 400);
}