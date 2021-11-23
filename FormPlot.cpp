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
    this->colors[1]  = QColor(0xff, 0x77, 0x33);
    this->colors[2]  = QColor(Qt::darkCyan);
    this->colors[3]  = QColor(Qt::red);
    this->colors[4]  = QColor(Qt::green);
    this->colors[5]  = QColor(Qt::yellow);
    this->colors[6]  = QColor(Qt::magenta);
    this->colors[7]  = QColor(Qt::gray);
    this->colors[8]  = QColor(Qt::darkYellow);
    this->colors[9]  = QColor(0x00, 0x33, 0x00);
    this->colors[10] = QColor(0x99, 0x33, 0x33);
    this->colors[11] = QColor(0x66, 0x99, 0xff);
    this->colors[12] = QColor(0x66, 0x33, 0x00);
    this->colors[13] = QColor(0xff, 0x66, 0xff);
    this->colors[14] = QColor(0x00, 0x00, 0x66);
    this->colors[15] = QColor(Qt::black);

    this->network = new QNetworkAccessManager();
    QObject::connect(network, SIGNAL(finished(QNetworkReply*)), this, SLOT(networkReplyReceived(QNetworkReply*)));

    this->ui->plot->setOpenGl(true);
    this->ui->plot->plotLayout()->clear();
    this->ui->plot->legend = new QCPLegend;
    this->ui->plot->legend->setVisible(true);
    this->ui->plot->legend->setSelectableParts(QCPLegend::spNone);
    this->plotAxis = new QCPAxisRect(this->ui->plot);
    this->plotAxis->setupFullAxesBox(true);
    this->plotAxis->axis(QCPAxis::atLeft, 0)->setTickLabels(true);
    for(int i = 0; i < this->pvList.count(); i++)
    {
        QString key = getUnit(this->pvList[i]);
        if(i == 0)
        {
            this->plotAxis->axis(QCPAxis::atLeft, 0)->setLabel(key);
            this->axisMap[key] = this->plotAxis->axis(QCPAxis::atLeft, 0);
        }
        else
        {
            this->axisMap[key] = this->plotAxis->addAxis(QCPAxis::atLeft);
            this->plotAxis->axis(QCPAxis::atLeft, i)->setLabel(key);
        }
    }
    this->ui->plot->plotLayout()->addElement(0, 0, this->plotAxis);

    QCPLayoutGrid *subLayout = new QCPLayoutGrid;
    ui->plot->plotLayout()->addElement(0, 1, subLayout);

    subLayout->addElement(0, 0, this->ui->plot->legend);
    subLayout->setMargins(QMargins(0, 0, 0, 0));
    ui->plot->legend->setFillOrder(QCPLegend::foRowsFirst);
    ui->plot->plotLayout()->setColumnStretchFactor(0, 9);
    ui->plot->plotLayout()->setColumnStretchFactor(1, 1);

    sendRequest();
}

FormPlot::~FormPlot()
{
    delete ui;
}

void FormPlot::plotData()
{
    QVector<double> xAxis, yAxis;
    for(auto item : qAsConst(this->pvData[0]))
    {
        xAxis.push_back(item.timestamp);
    }

    for (int i = 0; i < this->pvList.size(); i++) {
        yAxis.clear();
        for(auto item : qAsConst(this->pvData[i]))
            yAxis.push_back(item.value);

        QString axis = getUnit(this->pvList[i]);
        QCPGraph* graph = this->ui->plot->addGraph(this->plotAxis->axis(QCPAxis::atBottom), this->axisMap[axis]);
        graph->setLineStyle(QCPGraph::lsLine);
        graph->setPen(QPen(this->colors[i]));
        graph->setData(xAxis, yAxis);
        graph->setName(this->pvList[i]);
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
        url = QString(REQUEST_DATA_CSV).arg(
                this->pvList[i],
                this->ui->dtFrom->dateTime().toUTC().toString(ISO_DATETIME),
                this->ui->dtTo->dateTime().toUTC().toString(ISO_DATETIME),
                QString::number(sampling),
                processingMethod);

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
        else if(this->request.url().toString().contains("getPVDetails"))
        {
            return;
        }
        else
        {
            QMessageBox::information(0, "Error", "Invalid request");
            return;
        }
    }
}

QString FormPlot::getUnit(QString pv)
{
    QEventLoop loop;
    QNetworkReply* reply;
    QString url = QString(REQUEST_PV_DETAILS).arg(pv);
    this->request.setUrl(QUrl(url));
    reply = this->network->get(this->request);
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();
    if(reply->error() != QNetworkReply::NoError)
    {
        return "";
    }

    QJsonParseError error;
    QString response = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(response.toUtf8(), &error);
    if(error.error != QJsonParseError::NoError)
        return "";

    for(auto object : doc.array())
    {
        if(object.toObject()["name"] == "Units:")
            return object.toObject()["value"].toString();
    }
    return "";
}
