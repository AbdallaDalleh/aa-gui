#include "FormMain.h"
#include "ui_FormMain.h"

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
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        foreach(QJsonValue v, doc.array())
        {
            this->pvs.append(v.toString());
            this->ui->listBuffer->addItem(v.toString());
        }
        this->ui->listBuffer->sortItems();
    }
}

void FormMain::on_btnFetch_clicked()
{
    this->ui->listBuffer->clear();
    this->pvs.clear();
    this->request.setUrl(QUrl(REQUEST_PVS));
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

    return false;
}

void FormMain::on_btnAdd_clicked()
{
    this->ui->listData->addItem( this->ui->listBuffer->selectedItems()[0]->text() );
}

void FormMain::on_btnAddAll_clicked()
{
    foreach(QListWidgetItem* item, this->ui->listBuffer->selectedItems()) {
        this->ui->listData->addItem(item->text());
    }
}
