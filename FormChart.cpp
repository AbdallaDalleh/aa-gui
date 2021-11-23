#include "FormChart.h"
#include "ui_FormChart.h"

FormChart::FormChart(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::FormChart)
{
    ui->setupUi(this);

    QLineSeries* data;
    QLineSeries* data2, *data3;
    QChart* chart;

    data = new QLineSeries;
    data2 = new QLineSeries;
    data3 = new QLineSeries;
    qsrand(time(0));
    for (int i = 0; i < 50 ; i++) {
        data->append(i, qrand() % 10);
        data2->append(i, qrand() % 10);
        data3->append(i, qrand() % 10);
    }

    chart = new QChart;
    chart->legend()->hide();
    chart->setTitle("Test Data");
    // chart->createDefaultAxes();

    QValueAxis* x  = new QValueAxis;
    QValueAxis* y1 = new QValueAxis;
    QValueAxis* y2 = new QValueAxis;
    QValueAxis* y3 = new QValueAxis;

    x->setTickCount(10);
    y1->setTickCount(10);
    y2->setTickCount(10);
    y3->setTickCount(10);

    chart->addAxis(x, Qt::AlignBottom);
    chart->addAxis(y1, Qt::AlignLeft);
    chart->addAxis(y2, Qt::AlignRight);
    chart->addAxis(y3, Qt::AlignRight);

    chart->addSeries(data);
    chart->addSeries(data2);
    chart->addSeries(data3);

    data->attachAxis(x);
    data->attachAxis(y1);
    data2->attachAxis(x);
    data2->attachAxis(y2);
    data3->attachAxis(x);
    data3->attachAxis(y3);

    ui->chartView->setChart(chart);
    ui->chartView->setRenderHint(QPainter::Antialiasing);
}

FormChart::~FormChart()
{
    delete ui;
}
