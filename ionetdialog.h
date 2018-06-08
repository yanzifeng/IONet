#ifndef IONETDIALOG_H
#define IONETDIALOG_H

#include <QDialog>
#include <QtCharts/QChartView>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLineSeries>
#include <QtCharts/QLegend>
#include <QtCharts/QLegendMarker>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QLegend>
#include "udppwrapper.h"
#include <QQueue>
#include <QTimer>

QT_CHARTS_USE_NAMESPACE

namespace Ui {
class IONetDialog;
}

class IONetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit IONetDialog(QWidget *parent = 0);
    ~IONetDialog();
private slots:
    void setChannelTable(QString,QStringList);
    void setChannelTable(QString);
    void startTest();
    void stopTest();
    void setNetParams();
    void setAcqParams();
    void onTimer();

public slots:
    void desposeUdpData(QByteArray);
private:
    void setTableHeader();
    void createGraph();
    void readConfig();
private:
    Ui::IONetDialog *ui;
    QChart *chart;
    QValueAxis *axisX,*axisY;
    QLineSeries *seriesTest;
    QList<QLineSeries* > seriesList;
    UdpPWrapper *udp;
    bool mStart;
    QMap<QString,QString> mModeMap;
    QStringList acqrateList,channelList;
    int dt;
    QList<QPointF> dataPointList;
    QQueue<QByteArray> queue;
    QTimer *timer;
    QVector<QVector<QPointF>> allDataVector;
    int iCount,iRange;
    QList<int> rangeList;
    QString mBoardIP;
    int mBoardPort;
    int git;
};

#endif // IONETDIALOG_H
