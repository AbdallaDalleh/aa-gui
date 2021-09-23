#include "FormPlot.h"
#include "ui_FormPlot.h"

FormPlot::FormPlot(QStringList pvs, QDateTime from, QDateTime to, int sampling, QString processingMethod, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::FormPlot)
{
    ui->setupUi(this);

    this->pvList = pvs;
    this->ui->dtFrom->setDateTime(from);
    this->ui->dtTo->setDateTime(to);

    this->completer = new QCompleter(this->pvList, this);
    this->completer->setCaseSensitivity(Qt::CaseInsensitive);
    this->ui->txtPV->setCompleter(this->completer);
    this->sampling = sampling;
    this->processingMethod = processingMethod;

    this->colors[0]  = QColor(Qt::blue);
    this->colors[1]  = QColor("#ff7733");
    this->colors[2]  = QColor(Qt::darkCyan);
    this->colors[3]  = QColor(Qt::red);
    this->colors[4]  = QColor(Qt::green);
    this->colors[5]  = QColor(Qt::yellow);
    this->colors[6]  = QColor(Qt::magenta);
    this->colors[7]  = QColor(Qt::gray);
    this->colors[8]  = QColor(Qt::darkYellow);
    this->colors[9]  = QColor("#003300");
    this->colors[10] = QColor("#993333");
    this->colors[11] = QColor("#6699ff");
    this->colors[12] = QColor("#663300");
    this->colors[13] = QColor("#ff66ff");
    this->colors[14] = QColor("#000066");
    this->colors[15] = QColor(Qt::black);

//    this->ui->plot->plotLayout()->clear();
//    // this->ui->plot->setInteractions(/*QCP::iRangeZoom | */QCP::iRangeDrag | QCP::iSelectLegend | QCP::iMultiSelect);
//    this->ui->plot->legend = new QCPLegend;
//    this->ui->plot->legend->setVisible(true);
//    this->ui->plot->legend->setSelectableParts(QCPLegend::spNone);

    this->network = new QNetworkAccessManager();
    QObject::connect(network, SIGNAL(finished(QNetworkReply*)), this, SLOT(networkReplyReceived(QNetworkReply*)));

    sendRequest();
}

FormPlot::~FormPlot()
{
    delete ui;
}

void FormPlot::plotData()
{
    QVector<double> xAxis, yAxis;
    for(auto item : this->pvData[0])
    {
        xAxis.push_back(item.timestamp);
    }

    for (int i = 0; i < this->pvList.size(); i++) {
        yAxis.clear();
        QCPGraph* graph = this->ui->plot->addGraph();
        graph->setPen(QPen(this->colors[i]));
        for(auto item : this->pvData[i])
            yAxis.push_back(item.value);
        graph->setData(xAxis, yAxis);
        graph->keyAxis()->rescale();
        graph->valueAxis()->rescale();
    }
}

void FormPlot::sendRequest()
{
    QString url;
    QNetworkReply* reply;
    for(int i = 0; i < this->pvList.size(); i++)
    {
        QEventLoop loop;
        url = QString(REQUEST_DATA_CSV)
                .arg(this->pvList[i])
                .arg(this->ui->dtFrom->dateTime().toUTC().toString(ISO_DATETIME))
                .arg(this->ui->dtTo->dateTime().toUTC().toString(ISO_DATETIME))
                .arg(sampling)
                .arg(processingMethod);

        this->request.setUrl(QUrl(url));
        reply = this->network->get(this->request);
        QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
        loop.exec();
        if(reply->error() != QNetworkReply::NoError)
        {
            // TODO: QMessageBox
            return;
        }
    }

    int timestamp = this->pvData[0][0].timestamp;
    for (int i = 1; i < this->pvData.size(); i++)
    {
        if(this->pvData[i].size() > this->pvData[i - 1].size())
            timestamp = this->pvData[i][0].timestamp;
    }

    for(int i = 0; i < this->pvData.size(); i++)
    {
        for(int t = this->pvData[i][0].timestamp - this->sampling; t >= timestamp; t -= this->sampling)
        {
            this->pvData[i].insert(0, data_sample{t, 0.0});
        }
    }

    plotData();
}

void FormPlot::networkReplyReceived(QNetworkReply* reply)
{
    if(reply->error())
       QMessageBox::information(0, "Error", reply->errorString());
    else
    {
        if(this->request.url().toString().contains("getData.csv"))
        {
            QString raw = QString(reply->readAll());
            QStringList samples = raw.split('\n');
            if(samples[samples.size() - 1].isEmpty())
                samples.removeAt(samples.size() - 1);
            if(samples[samples.size() - 1].isEmpty())
                samples.removeAt(samples.size() - 1);

            QList<data_sample> list;
            for(QString sample : samples)
            {
                data_sample d;
                d.timestamp = sample.split(',')[0].toInt();
                d.value = sample.split(',')[1].toDouble();
                list.push_back(d);
            }
            pvData.push_back(list);
        }
        else
        {
            QMessageBox::information(0, "Error", "Invalid request");
            return;
        }
    }
}
