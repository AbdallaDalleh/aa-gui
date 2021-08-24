#ifndef FORMMAIN_H
#define FORMMAIN_H

#include <QMainWindow>
#include <QMessageBox>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>

#define SERVER_IP       "10.1.100.9"
#define RETRIEVAL_PORT  "17668"
#define MGMT_PORT       "17665"
#define REQUEST_PVS     "http://" SERVER_IP ":" MGMT_PORT "/mgmt/bpl/getAllPVs?limit=-1"

QT_BEGIN_NAMESPACE
namespace Ui { class FormMain; }
QT_END_NAMESPACE

class FormMain : public QMainWindow
{
    Q_OBJECT

public:
    FormMain(QWidget *parent = nullptr);
    ~FormMain();

    bool searchMatch(QString pv, QString arg);

private slots:
    void networkReplyReceived(QNetworkReply *reply);

    void on_btnFetch_clicked();

    void on_txtSearch_textChanged(const QString &arg1);

    void on_btnAdd_clicked();

    void on_btnAddAll_clicked();

private:
    Ui::FormMain *ui;

    QNetworkAccessManager* network;
    QNetworkRequest request;
    QStringList pvs;
};
#endif // FORMMAIN_H
