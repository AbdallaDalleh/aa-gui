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
    QList<QList<data_sample>> pvData;

    FormPlot* plot;
};
#endif // FORMMAIN_H
