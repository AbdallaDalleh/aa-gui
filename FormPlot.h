#ifndef FORMPLOT_H
#define FORMPLOT_H

#include <QMainWindow>
#include <QCompleter>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#include <qcustomplot.h>

#define SERVER_IP           "10.1.100.9"
#define RETRIEVAL_PORT      "17668"
#define MGMT_PORT           "17665"
#define REQUEST_DATA_CSV    "http://" SERVER_IP ":" RETRIEVAL_PORT "/retrieval/data/getData.csv?pv=%5_%4(%1)&from=%2&to=%3"
#define REQUEST_PV_DETAILS  "http://" SERVER_IP ":" MGMT_PORT "/mgmt/bpl/getPVDetails?pv=%1"
#define ISO_DATETIME        "yyyy-MM-ddThh:mm:ss.zzzZ"
#define AXIS_COUNT          5
#define GRAPH_COUNT         16
#define REQUEST_PVS_LIST    "http://" SERVER_IP ":" MGMT_PORT "/mgmt/bpl/getAllPVs?limit=-1"

struct data_sample
{
    int timestamp;
    double value;
};

namespace Ui {
class FormPlot;
}

class FormPlot : public QMainWindow
{
    Q_OBJECT

public:
    explicit FormPlot(QStringList pvs, QDateTime from, QDateTime to, int sampling, QString processingMethod, QWidget *parent = nullptr);
    ~FormPlot();

    void plotData();

    void sendRequest();

    QString getUnit(QString pv);

    void fillPVList();

    void setTickerFormat(uint duration, QSharedPointer<QCPAxisTickerDateTime> ticker);

    void saveScreenShot();

private slots:
    void networkReplyReceived(QNetworkReply *reply);

    void on_btnPlot_clicked();

    void on_btnAdd_clicked();

    void on_btnResetAxis_clicked();

    void on_btnResetGraph_clicked();

    void on_btnScreenshot_clicked();

    void onPlotDragFinished(QMouseEvent *event);

private:
    Ui::FormPlot *ui;

    int sampling;
    QStringList pvList;
    QStringList allPVs;
    QCompleter* completer;
    QList<QList<data_sample>> pvData;
    QNetworkAccessManager* network;
    QNetworkRequest request;
    QString processingMethod;
    QColor colors[GRAPH_COUNT];
    QMap<QString, QCPAxis*> axisMap;
    QSharedPointer<QCPAxisTickerDateTime> dateTicker;
    QString imageFileName;

    QCPAxisRect* plotAxis;
};

#endif // FORMPLOT_H
