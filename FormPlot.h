#ifndef FORMPLOT_H
#define FORMPLOT_H

#include <QMainWindow>
#include <QCompleter>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#include <qcustomplot.h>

#define SERVER_IP           "archiver.sesame.org.jo"
#define RETRIEVAL_PORT      "17668"
#define MGMT_PORT           "17665"
#define REQUEST_DATA_CSV    "http://" SERVER_IP ":" RETRIEVAL_PORT "/retrieval/data/getData.csv?pv=%5_%4(%1)&from=%2&to=%3"
#define REQUEST_DATA_LIVE   "http://" SERVER_IP ":" RETRIEVAL_PORT "/retrieval/data/getData.csv?pv=firstFill_1(%1)&from=%2&to=%3"
#define REQUEST_DATA_MAT    "http://" SERVER_IP ":" RETRIEVAL_PORT "/retrieval/data/getData.mat?pv=%1&from=%2&to=%3"
#define REQUEST_PV_DETAILS  "http://" SERVER_IP ":" MGMT_PORT "/mgmt/bpl/getPVDetails?pv=%1"
#define REQUEST_PVS_LIST    "http://" SERVER_IP ":" MGMT_PORT "/mgmt/bpl/getAllPVs?limit=-1"
#define ISO_DATETIME        "yyyy-MM-ddThh:mm:ss.zzzZ"
#define STANDARD_DATETIME   "hh:mm:ss dd/MM/yyyy"
#define AXIS_COUNT          5
#define GRAPH_COUNT         16
#define LIVE_DATA_PERIOD    900

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

    void onPlotZoomFinished(QWheelEvent* event);

    void onMouseMove(QMouseEvent* event);

    void onLegendClicked(QCPLegend *legend, QCPAbstractLegendItem *item, QMouseEvent *event);

    void on_btnExportCSV_clicked();

    void keyPressEvent(QKeyEvent* key);

    void on_btnLive_clicked();

public slots:
    void onLiveDataStart();

private:
    Ui::FormPlot *ui;

    int sampling;
    int selectedIndex;
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
    QCPAbstractLegendItem* selectedItem;
    QString selectedGraph;
    QTimer* liveData;

    QCPAxisRect* plotAxis;
};

#endif // FORMPLOT_H
