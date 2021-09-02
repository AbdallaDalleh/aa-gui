#ifndef FORMMAIN_H
#define FORMMAIN_H

#include <QMainWindow>
#include <QMessageBox>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFileDialog>

#include "FormPlot.h"

#define SERVER_IP           "10.1.100.9"
#define RETRIEVAL_PORT      "17668"
#define MGMT_PORT           "17665"
#define REQUEST_PVS_LIST    "http://" SERVER_IP ":" MGMT_PORT "/mgmt/bpl/getAllPVs?limit=-1"
#define REQUEST_DATA        "http://" SERVER_IP ":" RETRIEVAL_PORT "/retrieval/data/getData.json?pv=%5_%4(%1)&from=%2&to=%3"
#define REQUEST_DATA_CSV    "http://" SERVER_IP ":" RETRIEVAL_PORT "/retrieval/data/getData.csv?pv=%5_%4(%1)&from=%2&to=%3"
#define REQUEST_DATA_MAT    "http://" SERVER_IP ":" RETRIEVAL_PORT "/retrieval/data/getData.mat?pv=%1&from=%2&to=%3"
#define ISO_DATETIME        "yyyy-MM-ddThh:mm:ss.zzzZ"

enum OperationStatus
{
    InProgress,
    Success,
    Failed
};

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

    void setStatus(QString message, OperationStatus success);

private slots:
    void networkReplyReceived(QNetworkReply *reply);

    void on_btnFetch_clicked();

    void on_txtSearch_textChanged(const QString &arg1);

    void on_btnAdd_clicked();

    void on_btnAddAll_clicked();

    void on_btnRemove_clicked();

    void on_btnRemoveAll_clicked();

    void on_btnLoad_clicked();

    void on_btnSave_clicked();

    void on_btnNow_clicked();

    void on_btnExportCSV_clicked();

    void on_btnExportMAT_clicked();

    void on_btnPlotData_clicked();

private:
    Ui::FormMain *ui;

    QNetworkAccessManager* network;
    QNetworkRequest request;
    QStringList pvs;
    QList<QStringList> csvData;

    FormPlot* plot;
};
#endif // FORMMAIN_H
