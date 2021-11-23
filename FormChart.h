#ifndef FORMCHART_H
#define FORMCHART_H

#include <QMainWindow>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>

QT_CHARTS_USE_NAMESPACE

namespace Ui {
class FormChart;
}

class FormChart : public QMainWindow
{
    Q_OBJECT

public:
    explicit FormChart(QWidget *parent = nullptr);
    ~FormChart();

private:
    Ui::FormChart *ui;
};

#endif // FORMCHART_H
