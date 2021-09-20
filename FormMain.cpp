#include "FormMain.h"
#include "ui_FormMain.h"

#include <iostream>

FormMain::FormMain(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::FormMain)
{
    ui->setupUi(this);

    this->ui->dtFrom->setDateTime(QDateTime::currentDateTime().addSecs(-3600));
    this->ui->dtTo->setDateTime(QDateTime::currentDateTime());

    this->network = new QNetworkAccessManager();
    QObject::connect(network, SIGNAL(finished(QNetworkReply*)), this, SLOT(networkReplyReceived(QNetworkReply*)));

    this->on_btnFetch_clicked();

    this->plot = NULL;
}

FormMain::~FormMain()
{
    delete ui;
}

void FormMain::networkReplyReceived(QNetworkReply *reply)
{
    if(reply->error())
       QMessageBox::information(0, "Error", reply->errorString());
    else
    {
        if(this->request.url().toString().contains(REQUEST_PVS_LIST))
        {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            foreach(QJsonValue v, doc.array())
            {
                this->pvs.append(v.toString());
                this->ui->listBuffer->addItem(v.toString());
            }
            this->ui->listBuffer->sortItems();
        }
        else if(this->request.url().toString().contains("getData.csv"))
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
    }
}

void FormMain::on_btnFetch_clicked()
{
    this->ui->listBuffer->clear();
    this->pvs.clear();
    this->request.setUrl(QUrl(REQUEST_PVS_LIST));
    network->get(request);
}

void FormMain::on_txtSearch_textChanged(const QString &arg1)
{
    if(this->pvs.size() <= 0)
        return;

    if(arg1.length() == 0)
        this->on_btnFetch_clicked();

    this->ui->listBuffer->clear();
    foreach (QString pv, this->pvs) {
        if(searchMatch(pv, arg1))
            this->ui->listBuffer->addItem(pv);
    }
}

bool FormMain::searchMatch(QString pv, QString arg)
{
    if(this->ui->rbStartsWith->isChecked())
        return pv.startsWith(arg);
    else if(this->ui->rbEndsWith->isChecked())
        return pv.endsWith(arg);
    else if(this->ui->rbWildcard->isChecked())
    {
        QRegExp regex;
        regex.setPattern(arg);
        regex.setPatternSyntax(QRegExp::Wildcard);
        return regex.exactMatch(pv) || pv.contains(arg);
    }
    else if(this->ui->rbRegex->isChecked())
    {
        QRegularExpression regex;
        regex.setPattern(arg);
        return regex.match(pv).hasMatch();
    }

    return false;
}

void FormMain::on_btnAdd_clicked()
{
    QString pv = this->ui->listBuffer->selectedItems()[0]->text();
    if(this->ui->listData->findItems(pv, Qt::MatchExactly).empty())
        this->ui->listData->addItem( this->ui->listBuffer->selectedItems()[0]->text() );
}

void FormMain::on_btnAddAll_clicked()
{
    for (QListWidgetItem* item : this->ui->listBuffer->selectedItems()) {
        if(this->ui->listData->findItems(item->text(), Qt::MatchExactly).empty())
            this->ui->listData->addItem(item->text());
    }
}

void FormMain::on_btnRemove_clicked()
{
    if(this->ui->listData->selectedItems().size() > 0)
        delete this->ui->listData->selectedItems()[0];
}

void FormMain::on_btnRemoveAll_clicked()
{
    if(this->ui->listData->selectedItems().size() > 0)
    {
        for(QListWidgetItem* item : this->ui->listData->selectedItems())
            delete item;
    }
}

void FormMain::on_btnLoad_clicked()
{
    QFileDialog dialog;
    QString fileName = QFileDialog::getOpenFileName(this, "Select Template", getenv("HOME"), tr("Archiver Template Files (*.aat)"));
    if(fileName.isEmpty())
    {
        setStatus("No files selected.", Failed);
        return;
    }

    this->ui->txtName->setText(fileName);
    QFile file(fileName);
    if(!file.open(QFile::ReadOnly | QFile::Truncate))
    {
        setStatus("Could not open template: " + fileName, Failed);
    }

    this->ui->listData->clear();

    QRegExp regex("[.:TZ-]");
    QString line;
    QStringList items;
    QStringList timestampItems;
    int lineNumber = 0;
    while(!file.atEnd())
    {
        line = file.readLine();
        items = line.split(' ');
        if(items.size() != 2)
        {
            setStatus("Invalid template file, error parsing line " + QString::number(lineNumber), Failed);
            file.close();
            return;
        }

        if(items[0] == "pv")
            this->ui->listData->addItem(items[1].trimmed());
        else if(items[0] == "from" || items[0] == "to")
        {
            timestampItems = items[1].trimmed().split(regex);
            QDate date(timestampItems[0].toInt(), timestampItems[1].toInt(), timestampItems[2].toInt());
            QTime time(timestampItems[3].toInt(), timestampItems[4].toInt(), timestampItems[5].toInt(), timestampItems[6].toInt());
            QDateTime dt(date, time);
            if(items[0] == "from")
                this->ui->dtFrom->setDateTime(dt);
            else
                this->ui->dtTo->setDateTime(dt);
        }
        else
        {
            setStatus("Invalid configuration at line " + QString::number(lineNumber), Failed);
            return;
        }
    }

    setStatus("Template " + fileName + " loaded successfully.", Success);
}

void FormMain::setStatus(QString message, OperationStatus success)
{
    QPalette palette = this->statusBar()->palette();
    if(success == InProgress)
        palette.setColor(QPalette::Background, Qt::yellow);
    else if(success == Success)
        palette.setColor(QPalette::Background, Qt::green);
    else
        palette.setColor(QPalette::Background, Qt::red);

    this->statusBar()->setPalette(palette);
    this->statusBar()->setAutoFillBackground(true);
    this->statusBar()->showMessage(message);
}

void FormMain::on_btnSave_clicked()
{
    if(this->ui->dtFrom->dateTime().toTime_t() >= this->ui->dtTo->dateTime().toTime_t())
    {
        setStatus("Invalid date/time ranges", OperationStatus::Failed);
        return;
    }
    if(this->ui->listData->count() <= 0)
    {
        setStatus("No PVs selected", OperationStatus::Failed);
        return;
    }
    if(this->ui->txtName->text().length() <= 0)
    {
        setStatus("Empty template name", OperationStatus::Failed);
        return;
    }

    QString directory = QFileDialog::getExistingDirectory(this, "Select A Folder", getenv("HOME"));
    if(directory.isEmpty())
    {
        setStatus("Save directory not selected.", Failed);
        return;
    }

    QString fileName = directory + "/" + QFileInfo(this->ui->txtName->text()).fileName();
    if(!fileName.contains(".aat"))
        fileName += ".aat";

    QFile file(fileName);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        setStatus("Could not create file " + fileName, Failed);
        return;
    }

    QTextStream configTemplate(&file);
    for (int i = 0; i < this->ui->listData->count(); i++) {
        configTemplate << "pv " << this->ui->listData->item(i)->text() << endl;
    }

    configTemplate << "from " << this->ui->dtFrom->dateTime().toString(ISO_DATETIME) << endl;
    configTemplate << "to " << this->ui->dtTo->dateTime().toString(ISO_DATETIME) << endl;

    file.close();
    setStatus("Template " + fileName + " saved successfully", Success);
}

void FormMain::on_btnNow_clicked()
{
    this->ui->dtTo->setDateTime(QDateTime::currentDateTime());
}

void FormMain::on_btnExportCSV_clicked()
{
    QString processingMethod;
    QString url;
    QNetworkReply* reply;
    QFile csvFile;
    QTextStream csv;
    int interval;
    int sampling;

    for(auto item : this->pvData)
        item.clear();
    this->pvData.clear();

    if(this->ui->rbSecodns->isChecked())
        sampling = this->ui->sbPeriod->value();
    else if(this->ui->rbMinutes->isChecked())
        sampling = this->ui->sbPeriod->value() * 60;
    else
        sampling = this->ui->sbPeriod->value() * 3600;
    processingMethod = this->ui->cbMethod->currentIndex() == 0 ? "firstFill" : this->ui->cbMethod->currentText();
    interval = (this->ui->dtTo->dateTime().toTime_t() - this->ui->dtFrom->dateTime().toTime_t()) / sampling;

    QString fileName = QFileDialog::getSaveFileName(this, "Save CSV file", getenv("HOME"), tr("CSV Files (*.csv)"));
    if(fileName.isEmpty())
    {
        setStatus("No file selected", Failed);
        return;
    }

    csvFile.setFileName(fileName);
    csvFile.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate);
    csv.setDevice(&csvFile);
    csv << "Timestamp,";
    setStatus("Fetching data from the server ...", InProgress);
    this->ui->progressBar->setRange(0, this->ui->listData->count());
    for(int i = 0; i < this->ui->listData->count(); i++)
    {
        csv << this->ui->listData->item(i)->text();
        if(i != this->ui->listData->count() - 1)
            csv << ",";

        QEventLoop loop;
        url = QString(REQUEST_DATA_CSV)
                .arg(this->ui->listData->item(i)->text())
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
            setStatus("Error fetching data for PV " + this->ui->listData->item(i)->text(), Failed);
            csvFile.close();
            return;
        }

        this->ui->progressBar->setValue(i + 1);
    }

    setStatus("Exporting to CSV ...", InProgress);
    csv << endl;
    this->ui->progressBar->setRange(0, interval);
    int timestamp = this->pvData[0][0].timestamp;
    for (int i = 1; i < this->pvData.size(); i++)
    {
        if(this->pvData[i].size() > this->pvData[i - 1].size())
            timestamp = this->pvData[i][0].timestamp;
    }

    for(int i = 0; i < this->pvData.size(); i++)
    {
        for(int t = this->pvData[i][0].timestamp - sampling; t >= timestamp; t -= sampling)
        {
            this->pvData[i].insert(0, data_sample{t, 0.0});
        }
    }

    for(int t = 0; t < interval; t++)
    {
        csv << this->pvData[0][t].timestamp << ",";
        for(int i = 0; i < this->pvData.size(); i++)
        {
            csv << this->pvData[i][t].value;
            if(i != this->pvData.size() - 1)
                csv << ",";
        }
        csv << endl;
        this->ui->progressBar->setValue(t+1);
    }

    csvFile.close();
    setStatus("Data exported to " + fileName + " successfully.", Success);
}

void FormMain::on_btnExportMAT_clicked()
{
    QString processingMethod;
    QString url;
    QFile matFile;
    QNetworkReply* reply;
    int sampling;

    if(this->ui->rbSecodns->isChecked())
        sampling = this->ui->sbPeriod->value();
    else if(this->ui->rbMinutes->isChecked())
        sampling = this->ui->sbPeriod->value() * 60;
    else
        sampling = this->ui->sbPeriod->value() * 3600;
    processingMethod = this->ui->cbMethod->currentIndex() == 0 ? "firstFill" : this->ui->cbMethod->currentText();

    QString directory = QFileDialog::getExistingDirectory(0, "Select a Directory", getenv("HOME"));
    if(directory.isEmpty())
    {
        setStatus("Save directory not selected", Failed);
        return;
    }

    if(this->ui->listData->count() <= 0)
    {
        setStatus("No PVs selected", Failed);
        return;
    }

    setStatus("Fetching data from the server and exporting to MAT files ...", InProgress);
    this->ui->progressBar->setRange(0, this->ui->listData->count());
    for(int i = 0; i < this->ui->listData->count(); i++)
    {
        matFile.setFileName(directory + "/" + this->ui->listData->item(i)->text() + ".mat");
        if(!matFile.open(QIODevice::ReadWrite | QIODevice::Truncate))
        {
            setStatus("Could not open file " + matFile.fileName(), Failed);
            return;
        }

        QEventLoop loop;
        url = QString(REQUEST_DATA_MAT)
                .arg(this->ui->listData->item(i)->text())
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
            setStatus("Reply error for PV " + this->ui->listData->item(i)->text(), Failed);
            return;
        }

        matFile.write(reply->readAll());
        matFile.close();
        this->ui->progressBar->setValue(i + 1);
    }

    setStatus("Data exported successfully", Success);
}

void FormMain::on_btnPlotData_clicked()
{
    this->plot = new FormPlot(this->pvs, this->ui->dtFrom->dateTime(), this->ui->dtTo->dateTime(), this);
    this->plot->setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, this->plot->size(), qApp->desktop()->availableGeometry()));
    this->plot->show();
}
