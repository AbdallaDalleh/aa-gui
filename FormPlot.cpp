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

    this->network = new QNetworkAccessManager();
    QObject::connect(network, SIGNAL(finished(QNetworkReply*)), this, SLOT(networkReplyReceived(QNetworkReply*)));
    this->fillPVList();

    this->completer = new QCompleter(this->allPVs, this);
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
    this->plotAxis->setRangeDrag(Qt::Horizontal);
    this->ui->plot->plotLayout()->addElement(0, 0, this->plotAxis);

    QCPLayoutGrid *subLayout = new QCPLayoutGrid;
    ui->plot->plotLayout()->addElement(0, 1, subLayout);

    subLayout->addElement(0, 0, this->ui->plot->legend);
    subLayout->setMargins(QMargins(0, 0, 0, 0));
    ui->plot->legend->setFillOrder(QCPLegend::foRowsFirst);
    ui->plot->plotLayout()->setColumnStretchFactor(0, 9);
    ui->plot->plotLayout()->setColumnStretchFactor(1, 1);
    ui->plot->setInteractions(QCP::iRangeDrag);

    uint32_t start = this->ui->dtFrom->dateTime().toTime_t();
    uint32_t end = this->ui->dtTo->dateTime().toTime_t();
    dateTicker = QSharedPointer<QCPAxisTickerDateTime>(new QCPAxisTickerDateTime);
    setTickerFormat(end - start, dateTicker);

    QObject::connect(this->ui->plot, SIGNAL(mouseRelease(QMouseEvent*)), this, SLOT(onPlotDragFinished(QMouseEvent*)));

    sendRequest();
}

FormPlot::~FormPlot()
{
    delete ui;
}

// Plot the data to QCustomPlot.
void FormPlot::plotData()
{
    QVector<double> xAxis, yAxis;
    for(auto item : qAsConst(this->pvData[0]))
    {
        xAxis.push_back(item.timestamp);
    }

    this->ui->plot->clearGraphs();
    for (int i = 0; i < this->pvList.size(); i++) {
        yAxis.clear();
        for(auto item : qAsConst(this->pvData[i]))
            yAxis.push_back(item.value);

        QString axis = getUnit(this->pvList[i]);
        QCPGraph* graph = this->ui->plot->addGraph(this->plotAxis->axis(QCPAxis::atBottom), this->axisMap[axis]);
        graph->data()->clear();
        graph->setLineStyle(QCPGraph::lsLine);
        graph->setPen(QPen(this->colors[i % 16]));
        graph->setData(xAxis, yAxis);
        graph->setName(this->pvList[i]);
        graph->keyAxis()->rescale();
        graph->valueAxis()->rescale();

        setTickerFormat( ui->dtTo->dateTime().toTime_t() - ui->dtFrom->dateTime().toTime_t(), this->dateTicker );
        graph->keyAxis()->setTicker(this->dateTicker);
    }

    this->ui->plot->replot();
}

// The main function to request archived data.
void FormPlot::sendRequest()
{
    QString url;
    QNetworkReply* reply;
    uint32_t difference;

    difference = this->ui->dtTo->dateTime().toTime_t() - this->ui->dtFrom->dateTime().toTime_t();
    if(difference > 3600 * 8)
        sampling = (difference / (3600 * 8)) * 10;
    else
        sampling = 1;

    for(int i = 0; i < this->pvList.size(); i++)
    {
        QEventLoop loop;

        // Format the URL.
        url = QString(REQUEST_DATA_CSV).arg(
                this->pvList[i],
                this->ui->dtFrom->dateTime().toUTC().toString(ISO_DATETIME),
                this->ui->dtTo->dateTime().toUTC().toString(ISO_DATETIME),
                QString::number(sampling),
                processingMethod);

        // Perform the HTTP request and wait to finish.
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

    // This code is to handle a rare case where you request PV during a period which it was not
    // being archived, therefore we fill the missing timestamps with value 0.
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

// Main slot for receiving HTTP responses.
// This will check for all types of requests made to the archiver.
void FormPlot::networkReplyReceived(QNetworkReply* reply)
{
    if(reply->error())
       QMessageBox::information(0, "Error", reply->errorString());
    else
    {
        // Check if the request was done to fetch data, i.e. the url contains getData.csv
        // Parse CSV response and store the output in pvData, a list of lists of structs.
        // Each sublist is for a PV.
        if(this->request.url().toString().contains("getData.csv"))
        {
            QString raw = QString(reply->readAll());
            QStringList samples = raw.split('\n');
            if(samples[samples.size() - 1].isEmpty())
                samples.removeAt(samples.size() - 1);
            if(samples[samples.size() - 1].isEmpty())
                samples.removeAt(samples.size() - 1);

            QList<data_sample> list;
            for(QString sample : qAsConst(samples))
            {
                data_sample d;
                d.timestamp = sample.split(',')[0].toInt();
                d.value = sample.split(',')[1].toDouble();
                list.push_back(d);
            }
            pvData.push_back(list);
        }

        // Check if the request was to read the PVs list, parse it and store it in this->pvs.
        else if(this->request.url().toString().contains("getAllPVs"))
        {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            foreach(QJsonValue v, doc.array())
            {
                this->allPVs.append(v.toString());
            }
        }

        // Check if we received a getPVDetails request. Do nothing and returen.
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

void FormPlot::on_btnPlot_clicked()
{
    for(auto list : qAsConst(this->pvData))
        list.clear();
    this->pvData.clear();
    sendRequest();
    this->ui->plot->replot();
}

void FormPlot::fillPVList()
{
    QEventLoop loop;
    this->request.setUrl(QUrl(REQUEST_PVS_LIST));
    QNetworkReply* reply = network->get(this->request);
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();
}

void FormPlot::on_btnAdd_clicked()
{
    QString url;
    QString key;
    QString pv;
    QEventLoop loop;
    QNetworkReply* reply;

    pv = this->ui->txtPV->text();
    if(this->pvList.contains(pv))
    {
        QMessageBox::information(this, "Note", "PV already exists.");
        return;
    }

    key = getUnit(pv);
    if(!this->axisMap.contains(key))
    {
        this->axisMap[key] = this->plotAxis->addAxis(QCPAxis::atLeft);
        this->plotAxis->axis(QCPAxis::atLeft, this->pvList.size())->setLabel(key);
    }

    this->pvList.append(pv);
    url = QString(REQUEST_DATA_CSV).arg(
            pv,
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
        QMessageBox::warning(this, "Error", "Could not fetch data for PV: " + this->ui->txtPV->text());
        return;
    }

    // This code is to handle a rare case where you request PV during a period which it was not
    // being archived, therefore we fill the missing timestamps with value 0.
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

void FormPlot::setTickerFormat(uint duration, QSharedPointer<QCPAxisTickerDateTime> ticker)
{
    if(duration < 24 * 3600)
        ticker->setDateTimeFormat("ddd hh:mm");
    else if(duration >= 24 * 3600 && duration < 7 * 24 * 3600)
        ticker->setDateTimeFormat("dd/MM");
    else if(duration >= 7 * 24 * 3600 && duration < 6 * 30 * 24 * 3600)
        ticker->setDateTimeFormat("MMM dd");
    else
        ticker->setDateTimeFormat("MMM YYYY");

    dateTicker->setTickCount(7);
    dateTicker->setTickStepStrategy(QCPAxisTicker::tssReadability);
}

void FormPlot::on_btnResetAxis_clicked()
{
    for(int i = 0; i < this->ui->plot->graphCount(); i++)
    {
        this->ui->plot->graph(i)->valueAxis()->rescale();
        this->ui->plot->graph(i)->keyAxis()->rescale();
    }

    this->ui->plot->replot();
}

void FormPlot::on_btnResetGraph_clicked()
{
    this->ui->dtFrom->setDateTime(QDateTime::currentDateTime().addSecs(-3600));
    this->ui->dtTo->setDateTime(QDateTime::currentDateTime());
    for(auto list : qAsConst(this->pvData))
        list.clear();
    this->pvData.clear();
    sendRequest();
}

void FormPlot::on_btnScreenshot_clicked()
{
    QString initialPath = "/home/control/nfs/machine/screenshots";
    QFileDialog fileDialog(this, tr("Save As"), initialPath);
    fileDialog.setAcceptMode(QFileDialog::AcceptSave);
    fileDialog.setFileMode(QFileDialog::AnyFile);
    fileDialog.setDirectory(initialPath);
    fileDialog.setDefaultSuffix(".png");
    if(fileDialog.exec() != QDialog::Accepted)
        return;

    this->imageFileName = fileDialog.selectedFiles().at(0);
    QTimer::singleShot(500, this, &FormPlot::saveScreenShot);
}

void FormPlot::saveScreenShot()
{
    QApplication::beep();
    this->ui->plot->savePng(this->imageFileName);
}

void FormPlot::onPlotDragFinished(QMouseEvent *event)
{
    Q_UNUSED(event);

    QCPAxis* xAxis = this->ui->plot->xAxis;

    this->ui->dtFrom->setDateTime(QDateTime::fromTime_t(xAxis->range().lower));
    this->ui->dtTo->setDateTime(QDateTime::fromTime_t(xAxis->range().upper));
    for(auto list : qAsConst(this->pvData))
        list.clear();
    this->pvData.clear();
    sendRequest();
}
