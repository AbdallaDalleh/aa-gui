#include "FormPlot.h"
#include "ui_FormPlot.h"

#include <iostream>
using std::cout;
using std::endl;

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
            if(!this->axisMap.contains(key))
            {
                this->axisMap[key] = this->plotAxis->addAxis(QCPAxis::atLeft);
                this->axisMap[key]->setLabel(key);
            }
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
    ui->plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectLegend);
    this->selectedItem = NULL;
    this->selectedGraph = "";

    uint32_t start = this->ui->dtFrom->dateTime().toTime_t();
    uint32_t end = this->ui->dtTo->dateTime().toTime_t();
    dateTicker = QSharedPointer<QCPAxisTickerDateTime>(new QCPAxisTickerDateTime);
    setTickerFormat(end - start, dateTicker);

    QObject::connect(this->ui->plot, SIGNAL(mouseRelease(QMouseEvent*)), this, SLOT(onPlotDragFinished(QMouseEvent*)));
    QObject::connect(this->ui->plot, SIGNAL(mouseWheel(QWheelEvent*)), this, SLOT(onPlotZoomFinished(QWheelEvent*)));
    QObject::connect(this->ui->plot, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(onMouseMove(QMouseEvent*)));
    QObject::connect(this->ui->plot, SIGNAL(legendClick(QCPLegend*,QCPAbstractLegendItem*,QMouseEvent*)), this, SLOT(onLegendClicked(QCPLegend*,QCPAbstractLegendItem*,QMouseEvent*)));

    sendRequest();

    this->liveData = new QTimer(this);
    QObject::connect(liveData, SIGNAL(timeout()), this, SLOT(onLiveDataStart()));
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
    setTickerFormat( ui->dtTo->dateTime().toTime_t() - ui->dtFrom->dateTime().toTime_t(), this->dateTicker );
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
        graph->keyAxis()->setTicker(this->dateTicker);
        graph->keyAxis()->rescale();
        graph->valueAxis()->rescale();

        QCPRange range = graph->valueAxis()->range();
        graph->valueAxis()->setRange(range.lower * 0.95, range.upper * 1.05);
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

        // Perform the HTTP request and use the event loop to wait to finish.
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
    // being archived, therefore we fill the missing timestamps with value 0, more like firstFill.
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
        // Check if the request was done to fetch data, i.e. the url contains getData.csv.
        // Parse CSV response and store the output in pvData, a list of lists of structs.
        // Each sublist is for a PV.
        if(this->request.url().toString().contains("getData.csv") && this->request.url().toString().contains("to"))
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

        // Check if the request was to read the PVs list, parse it and store it in this->allPVs.
        else if(this->request.url().toString().contains("getAllPVs"))
        {
            QJsonParseError error;
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &error);
            if(error.error != QJsonParseError::NoError)
            {
                QMessageBox::warning(this, "Error", "Could not read all PVs list");
                return;
            }

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

// Read the PV's unit by reading the PV's details and parse the JSON response.
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

    foreach (auto object, doc.array())
    {
        if(object.toObject()["name"] == "Units:")
            return object.toObject()["value"].toString();
    }
    return "";
}

// The btnPlot is used to plot data after changing start/end timestamps.
void FormPlot::on_btnPlot_clicked()
{
    if(this->pvList.isEmpty())
    {
        QMessageBox::warning(this, "Error", "Empty PV List.");
        return;
    }

    // Basically, quit live mode by enabling all buttons, enable interactions
    // and stop the timer.
    ui->btnAdd->setEnabled(true);
    ui->btnExportCSV->setEnabled(true);
    ui->btnLive->setEnabled(true);
    ui->btnResetAxis->setEnabled(true);
    ui->btnResetGraph->setEnabled(true);
    ui->btnScreenshot->setEnabled(true);
    ui->plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectLegend);
    this->liveData->stop();

    // Clear all current data.
    for(auto list : qAsConst(this->pvData))
        list.clear();
    this->pvData.clear();

    if(ui->plot->xAxis->range().upper > QDateTime::currentSecsSinceEpoch())
        ui->dtTo->setDateTime(QDateTime::currentDateTime());
    sendRequest();
}

void FormPlot::fillPVList()
{
    QEventLoop loop;
    this->request.setUrl(QUrl(REQUEST_PVS_LIST));
    QNetworkReply* reply = network->get(this->request);
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();
}

// Add PV to the plot from the search text box.
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

    // From here on, this is basically same as sendRequest but for a single PV.
    key = getUnit(pv);
    this->pvList.append(pv);
    if(!this->axisMap.contains(key))
    {
        if(this->pvList.size() == 1)
        {
            this->plotAxis->axis(QCPAxis::atLeft, 0)->setLabel(key);
            this->axisMap[key] = this->plotAxis->axis(QCPAxis::atLeft, 0);
        }
        else
        {
            this->axisMap[key] = this->plotAxis->addAxis(QCPAxis::atLeft);
            this->axisMap[key]->setLabel(key);
        }
    }

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

// Calculate the duration of the plot and change the ticker format accordingly.
void FormPlot::setTickerFormat(uint duration, QSharedPointer<QCPAxisTickerDateTime> ticker)
{
    if(duration < 24 * 3600)
        ticker->setDateTimeFormat("ddd hh:mm");
    else if(duration >= 24 * 3600 && duration < 7 * 24 * 3600)
        ticker->setDateTimeFormat("dd/MM");
    else if(duration >= 7 * 24 * 3600 && duration < 6 * 30 * 24 * 3600)
        ticker->setDateTimeFormat("MMM dd");
    else
        ticker->setDateTimeFormat("MMM yyyy");

    dateTicker->setTickCount(7);
    dateTicker->setTickStepStrategy(QCPAxisTicker::tssReadability);
}

// Rescale all axis and reset the start/end timestamps.
void FormPlot::on_btnResetAxis_clicked()
{
    for(int i = 0; i < this->ui->plot->graphCount(); i++)
    {
        this->ui->plot->graph(i)->valueAxis()->rescale();
        this->ui->plot->graph(i)->keyAxis()->rescale();
    }

    this->ui->dtFrom->setDateTime(QDateTime::fromTime_t(this->ui->plot->xAxis->range().lower));
    this->ui->dtTo->setDateTime(QDateTime::fromTime_t(this->ui->plot->xAxis->range().upper));
    this->ui->plot->replot();
}

// Reset the graph to a 1 hour default plot of data.
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

    // Check if the current mouse position is outside the the axis rectangle, i.e. the plot itself.
    // pixelToCoord maps the given x coordinate to the current X axis value and check if we are outside
    // the X axis. Same thing for the Y axis.
    QCPAxis* xAxis = this->ui->plot->xAxis;
    if(xAxis->pixelToCoord(event->pos().x()) < ui->dtFrom->dateTime().toTime_t() ||
       xAxis->pixelToCoord(event->pos().x()) > ui->dtTo->dateTime().toTime_t())
        return;

    // Check if the X axis upper range is larger than the current time, reading data from the from the future.
    // (Yeah good luck with that :| )
    if((uint32_t) xAxis->range().upper > QDateTime::currentSecsSinceEpoch())
    {
        QMessageBox::information(this, "Wow!", "Good luck trying to read \"archived\" data from the future :)");
        return;
    }

    // Reset dtFrom and dtTo to the current X axis upper range.
    this->ui->dtFrom->setDateTime(QDateTime::fromTime_t(xAxis->range().lower));
    this->ui->dtTo->setDateTime(QDateTime::fromTime_t(xAxis->range().upper));
    for(auto list : qAsConst(this->pvData))
        list.clear();
    this->pvData.clear();
    sendRequest();
}

// Catch zoom on the plot and set the from/to datetime from the x axis accordingly.
void FormPlot::onPlotZoomFinished(QWheelEvent* event)
{
    Q_UNUSED(event);

    QCPAxis* xAxis = this->ui->plot->xAxis;
    this->ui->dtFrom->setDateTime(QDateTime::fromTime_t((uint32_t)xAxis->range().lower));
    this->ui->dtTo->setDateTime(QDateTime::fromTime_t((uint32_t)xAxis->range().upper));
    setTickerFormat((uint32_t)xAxis->range().upper - (uint32_t)xAxis->range().lower, this->dateTicker);
}

// Catch the mouse move event to draw the tooltip.
void FormPlot::onMouseMove(QMouseEvent* event)
{
    QString message;
    time_t epoch;
    double value;
    QCPGraph* graph;

    // Again check if the current mouse position is outside the the axis rectangle, i.e. the plot itself.
    // same as plotDragFinished.
    if(ui->plot->xAxis->pixelToCoord(event->pos().x()) < ui->dtFrom->dateTime().toTime_t() ||
       ui->plot->xAxis->pixelToCoord(event->pos().x()) > ui->dtTo->dateTime().toTime_t())
        return;

    // Get the timestamp of the current mouse position.
    epoch = ui->plot->axisRect()->axis(QCPAxis::atBottom, 0)->pixelToCoord(event->pos().x());
    message = "Timestamp - " + QDateTime::fromTime_t(epoch).toString("hh:mm:ss dd/MM/yyyy") + "\n";

    for(int i = 0; i < ui->plot->graphCount(); i++)
    {
        // Check if the mouse position is outside the Y axis of this graph.
        graph = ui->plot->graph(i);
        if(graph->valueAxis()->pixelToCoord(event->pos().y()) > graph->valueAxis()->range().upper ||
           graph->valueAxis()->pixelToCoord(event->pos().y()) < graph->valueAxis()->range().lower)
            return;

        // Used the graph data iterator to find the current value by mapping the mouse positions' Y
        // coordinate to the current axis Y value using pixeltoCoord
        value = graph->data()->findBegin(graph->keyAxis()->pixelToCoord(event->pos().x()))->value;
        message += graph->name() + " - " + QString::number(value) + " " + graph->valueAxis()->label() + (i == ui->plot->graphCount() - 1 ? "" : "\n");
    }

    QToolTip::showText(ui->plot->mapToGlobal(event->pos()), message);
}

// Catch mouse click on the plot's legend.
void FormPlot::onLegendClicked(QCPLegend *legend, QCPAbstractLegendItem *item, QMouseEvent *event)
{
    Q_UNUSED(event)

    if(event->button() == Qt::RightButton)
        return;

    for(int i = 0; i < legend->itemCount(); i++)
    {
        if(item == legend->item(i))
        {
            // Save the selected graph name and index for use in keyPressEvent.
            if(this->ui->plot->graph(i)->visible())
            {
                this->selectedIndex = i;
                this->selectedGraph = ui->plot->graph(i)->name();
            }
            else
            {
                this->selectedIndex = -1;
                this->selectedGraph = "";
            }

            // Hide the corresponding graph and gray-out the legend's text.
            this->ui->plot->graph(i)->setVisible( ! this->ui->plot->graph(i)->visible() );
            item->setTextColor( this->ui->plot->graph(i)->visible() ? Qt::black : Qt::gray );
            break;
        }
    }

    this->ui->plot->replot();
}

void FormPlot::on_btnExportCSV_clicked()
{
    QMessageBox::information(this, "He", QString::number(this->pvData[0].size()));
    QFile csvFile;
    QTextStream csv;
    QString fileName;

    fileName = QFileDialog::getSaveFileName(this, "Save CSV file", getenv("HOME"), tr("CSV Files (*.csv)"));
    if(fileName.isEmpty())
    {
        return;
    }

    if(!fileName.endsWith(".csv"))
        fileName += ".csv";

    csvFile.setFileName(fileName);
    csvFile.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate);
    csv.setDevice(&csvFile);
    csv << "Timestamp,";
    for(int i = 0; i < ui->plot->graphCount(); i++)
    {
        csv << ui->plot->graph(i)->name();
        if(i != ui->plot->graphCount() - 1)
            csv << ",";
    }
    csv << endl;
    for(int i = 0; i < this->pvData[0].size(); i++)
    {
        csv << QDateTime::fromSecsSinceEpoch(this->pvData[0][i].timestamp).toString(STANDARD_DATETIME) << ",";
        for(int j = 0; j < this->pvData.size(); j++)
        {
            csv << pvData[j][i].value;
            if(j != this->pvData.size() - 1)
                csv << ",";
        }
        csv << endl;
    }

    csvFile.close();
    QMessageBox::information(this, "Done", "CSV File exported to " + fileName + " successfully.");
}

void FormPlot::keyPressEvent(QKeyEvent *event)
{
//    // Return if live mode is running.
//    if(this->liveData->isActive())
//        return;

//    // Check for the delete key only.
//    if(event->key() == Qt::Key_Delete && !this->selectedGraph.isEmpty() && this->selectedIndex != -1)
//    {
//        this->pvList.removeOne(this->selectedGraph);
//        this->pvData.removeAt(this->selectedIndex);
//        ui->plot->removeGraph(this->selectedIndex);
//        foreach (auto axis, this->plotAxis->axes())
//        {
//            if(axis->graphs().count() <= 0)
//            {
//                this->plotAxis->removeAxis(axis);
//                this->axisMap.remove(axis->label());
//            }
//        }
//        this->plotAxis->setupFullAxesBox(true);
//    }

//    ui->plot->replot();
//    QMainWindow::keyPressEvent(event);
}

// Start live data mode.
void FormPlot::on_btnLive_clicked()
{
    // Disable all buttons except btnPlot.
    ui->btnAdd->setEnabled(false);
    ui->btnExportCSV->setEnabled(false);
    ui->btnLive->setEnabled(false);
    ui->btnResetAxis->setEnabled(false);
    ui->btnResetGraph->setEnabled(false);
    ui->btnScreenshot->setEnabled(false);

    // Clear plot's graphs and interactions for proper live data display.
    ui->plot->clearGraphs();
    this->plotAxis->setupFullAxesBox(true);
    this->pvData.clear();
    ui->plot->setInteraction(QCP::iRangeDrag, false);
    ui->plot->setInteraction(QCP::iRangeZoom, false);
    ui->plot->setInteraction(QCP::iSelectLegend, false);
    ui->plot->replot();

    // Start a 900 seconds data frame request and start the timer.
    this->ui->dtFrom->setDateTime(QDateTime::currentDateTime().addSecs(-LIVE_DATA_PERIOD));
    this->ui->dtTo->setDateTime(QDateTime::currentDateTime());
    sendRequest();
    liveData->start(1000);
}

void FormPlot::onLiveDataStart()
{
    QString url;
    QEventLoop loop;
    QNetworkReply* reply;

    for(auto list : qAsConst(this->pvData))
        list.clear();
    this->pvData.clear();

    // Fetch the 900 seconds data frame for all PVs and plot the data.
    foreach (QString pv, this->pvList)
    {
        url = QString(REQUEST_DATA_LIVE).arg(pv,
                                             QDateTime::currentDateTimeUtc().addSecs(-LIVE_DATA_PERIOD).toString(ISO_DATETIME),
                                             QDateTime::currentDateTimeUtc().toString(ISO_DATETIME));
        this->request.setUrl(QUrl(url));
        reply = this->network->get(request);
        QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
        loop.exec();
        if(reply->error() != QNetworkReply::NoError)
        {
            QMessageBox::warning(this, "Error", "Could not fetch live data.");
            return;
        }
    }

    plotData();
}
