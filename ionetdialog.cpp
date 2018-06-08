#include "ionetdialog.h"
#include "ui_ionetdialog.h"
#include <QDebug>
#include <QDataStream>
#include <QThread>
#include <QtEndian>
#include <QTime>
#include <QSettings>
#include <QTextCodec>

IONetDialog::IONetDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IONetDialog)
{
    ui->setupUi(this);
    readConfig();
    mModeMap["0301"] = "SNET01";
    mModeMap["0302"] = "SNET02";
    acqrateList << "250kS/s" << "125kS/s" << "62.5kS/s" << "31.25kS/s" << "15.625kS/s" << "7.8125kS/s";
    setWindowFlags(Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    connect(ui->comboBox_ch,SIGNAL(currentTextChanged(QString)),this,SLOT(setChannelTable(QString)));
    setTableHeader();
    createGraph();
    for(int i=0;i<600;i++)
    {
        dataPointList << QPointF(double(i)/600.0,double(i)*double(i)/360000.0);
    }
    setChannelTable("1");

    udp = new UdpPWrapper(ui->spinBox_PCPort->value());
    QThread *thread = new QThread;
    udp->moveToThread(thread);
    thread->start();

    connect(ui->pushButton_setacq,SIGNAL(clicked(bool)),this,SLOT(setAcqParams()));
    connect(ui->pushButton_setnet,SIGNAL(clicked(bool)),this,SLOT(setNetParams()));
    connect(ui->pushButton_start,SIGNAL(clicked(bool)),this,SLOT(startTest()));
    connect(ui->pushButton_stop,SIGNAL(clicked(bool)),this,SLOT(stopTest()));
    connect(udp,SIGNAL(sendAcqData(QByteArray)),this,SLOT(desposeUdpData(QByteArray)));

    mStart = false;

    timer = new QTimer(this);
    timer->setInterval(1);
    connect(timer,SIGNAL(timeout()),this,SLOT(onTimer()));
    rangeList << 400 << 200 << 100 << 50 << 40 << 20;
    mBoardIP = ui->lineEdit_boartIP->text();
    mBoardPort = ui->spinBox_BoardPort->value();
}

IONetDialog::~IONetDialog()
{
    timer->stop();
    delete timer;
    delete udp;
    delete ui;
}

void IONetDialog::setChannelTable(QString value,QStringList chList)
{
    ui->tableWidget->setRowCount(0);
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(value.toInt());
    for(int i=0;i<ui->tableWidget->rowCount();i++)
    {
        QTableWidgetItem *item = new QTableWidgetItem(QString::number(i+1));
        ui->tableWidget->setItem(i,0,item);
        item->setTextAlignment(Qt::AlignCenter);

        QSpinBox *spinBox = new QSpinBox;
        spinBox->setMaximum(11); spinBox->setMinimum(0);
        spinBox->setAlignment(Qt::AlignCenter);
        ui->tableWidget->setCellWidget(i,1,spinBox);
        spinBox->setValue(chList.at(i).toInt());
        spinBox->setStyleSheet("border:1px lightgray");
    }
    chart->removeAllSeries();
    seriesList.clear();
    for(int i=0;i<value.toInt();i++)
    {
        QLineSeries *series = new QLineSeries;
        series->setUseOpenGL(true);
        seriesList.append(series);
        series->setName(QStringLiteral("通道%1").arg(chList.at(i)));
        chart->addSeries(series);
        chart->setAxisX(axisX,series);
        chart->setAxisY(axisY,series);
    }
    auto const markers = chart->legend()->markers();
    for (QLegendMarker *marker : markers)
    {
        marker->setVisible(true);
        marker->setFont(QFont(QStringLiteral("微软雅黑"),10));
    }
}
void IONetDialog::setChannelTable(QString value)
{
    ui->tableWidget->setRowCount(0);
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(value.toInt());
    for(int i=0;i<ui->tableWidget->rowCount();i++)
    {
        QTableWidgetItem *item = new QTableWidgetItem(QString::number(i+1));
        ui->tableWidget->setItem(i,0,item);
        item->setTextAlignment(Qt::AlignCenter);

        QSpinBox *spinBox = new QSpinBox;
        spinBox->setMaximum(11); spinBox->setMinimum(0);
        spinBox->setAlignment(Qt::AlignCenter);
        ui->tableWidget->setCellWidget(i,1,spinBox);
        spinBox->setValue(i);
        spinBox->setStyleSheet("border:1px lightgray");
    }
    chart->removeAllSeries();
    seriesList.clear();
    for(int i=0;i<value.toInt();i++)
    {
        QLineSeries *series = new QLineSeries;
        series->append(dataPointList);
        seriesList.append(series);
        series->setName(QStringLiteral("通道%1").arg(i));
        chart->addSeries(series);
        chart->setAxisX(axisX,series);
        chart->setAxisY(axisY,series);
    }
    auto const markers = chart->legend()->markers();
    for (QLegendMarker *marker : markers)
    {
        marker->setVisible(true);
        marker->setFont(QFont(QStringLiteral("微软雅黑"),10));
    }
}
void IONetDialog::setTableHeader()
{
    QStringList list;
    list << QStringLiteral("通道号") << QStringLiteral("通道选择");
    ui->tableWidget->setHorizontalHeaderLabels(list);
    ui->tableWidget->verticalHeader()->setVisible(false);
    ui->tableWidget->horizontalHeader()->setVisible(true);
    QList<int> hSize;
    hSize << 80 << 100;
    for(int i=0; i<ui->tableWidget->columnCount();i++)
    {
        ui->tableWidget->setColumnWidth(i,hSize.at(i));
    }
}
void IONetDialog::createGraph()
{
    QFont font(QStringLiteral("微软雅黑"),10);

    chart = new QChart();
//    chart->legend()->hide();
    chart->createDefaultAxes();
    ui->chartView->setChart(chart);


    axisX = new QValueAxis;
    axisX->setRange(0, 0.1);
    axisX->setLabelFormat("%.2f");
    axisX->setTitleText(QStringLiteral("时间(s)"));
    axisX->setLabelsFont(font);
    axisX->setVisible(true);
    axisX->setTickCount(10);
//    axisX->hide();

    axisY = new QValueAxis;
    axisY->setRange(-10, 10);
    axisY->setLabelFormat("%.2f");
    axisY->setTitleText(QStringLiteral("幅度(V)"));
    axisY->setLabelsFont(font);
    axisY->setVisible(true);
    axisY->setTickCount(5);

    chart->setAxisX(axisX);
    chart->setAxisY(axisY);
}
void IONetDialog::startTest()
{
    mStart = false;
    udp->startAcq(QHostAddress(ui->lineEdit_boartIP->text()),ui->spinBox_BoardPort->value());
    timer->start();
    queue.clear();
}
void IONetDialog::stopTest()
{
    udp->stopAcq(QHostAddress(ui->lineEdit_boartIP->text()),ui->spinBox_BoardPort->value());
    timer->stop();
}
void IONetDialog::setNetParams()
{
    QStringList list;
    list << ui->lineEdit_PCIP->text() << QString::number(ui->spinBox_PCPort->value()) << ui->lineEdit_boartIP->text() << QString::number(ui->spinBox_BoardPort->value());
    udp->setNetParams(list,QHostAddress(mBoardIP),mBoardPort);
}
void IONetDialog::setAcqParams()
{
//    udp->setAcqParams(ui->comboBox);
    QStringList list;
    for(int i=0;i<ui->tableWidget->rowCount();i++)
    {
        QSpinBox *spinBox = (QSpinBox*)ui->tableWidget->cellWidget(i,1);
        list.append(QString::number(spinBox->value()));
    }
    udp->setAcqParams(ui->comboBox_acq->currentIndex()+1,list,QHostAddress(ui->lineEdit_boartIP->text()),ui->spinBox_BoardPort->value());
}
void IONetDialog::desposeUdpData(QByteArray ba)
{
    if(ba.size() == 1220 && ba.left(2).toHex().toUpper() == "ABAB")
    {
        QString headerstring = ba.left(20).toHex().toUpper();//ABAB 0001 0301 0001 0001 000F 00 00000000000000
//        qDebug() << "Frame Number : "<< headerstring.mid(4,4).toInt(0,16);
        if(!mStart) //decode header
        {

            mStart = true;
            ui->lineEdit->setText(mModeMap.value(headerstring.mid(8,4)));
            ui->lineEdit_4->setText(acqrateList.at(headerstring.mid(16,4).toInt(0,16)-1));
            ui->comboBox_acq->setCurrentIndex(headerstring.mid(16,4).toInt(0,16)-1);
            dt = pow(2,headerstring.mid(16,4).toInt(0,16)-1)*4;
            QString chBinString = QString::number(headerstring.mid(20,4).toInt(0,16),2);//1111  //"110100101111" (0,1,2,3,5,8,10,11)
            qDebug() << "enter into header decode";
            channelList.clear();
            for(int i=0;i<chBinString.size();i++)
            {
                if(chBinString.at(i) == '1')
                {
                    channelList.insert(0,QString::number(chBinString.size()-i-1));
                }
            }
            qDebug() << channelList;
            allDataVector.clear();
            ui->comboBox_ch->setCurrentText(QString::number(channelList.size()));
            setChannelTable(ui->comboBox_ch->currentText(),channelList);
            for(int i=0;i<ui->comboBox_ch->currentText().toInt();i++)
            {
                QVector<QPointF> vector;
                allDataVector.append(vector);
            }
            iCount = 0;
            iRange = rangeList.at(ui->comboBox_acq->currentIndex()) * 2;
        }
        else  //decode data
        {
            if(queue.size() > 100)
            {
                queue.clear();
            }
            queue.enqueue(ba.mid(20));
            ui->lineEdit_5->setText(tr("Queue Size : %1").arg(queue.size()));
        }
    }
}
void IONetDialog::onTimer()
{
    int queuesize = queue.size();
//    qDebug() << "queue size : " << queue.size();
    QTime time;
    time.start();
    for(int k=0;k<queuesize;k++)
    {
        QByteArray ba = queue.dequeue();
//        qDebug() << "enter into data decode";
        qint16 array[600];
        memcpy(array,ba.data(),1200);
        int ch = ui->comboBox_ch->currentText().toInt();
        int datasize = 600/ch;
//        qDebug() << ch << datasize;
 /*
        for(int i=0;i<ch;i++)
        {
            QList<QPointF> tlist;
            for(int j=0;j<datasize;j++)
            {
                tlist << QPointF(qreal(j)*(qreal)dt/1000000.0,(double)(qToBigEndian(array[j*ch + i]))*0.305/1000.0);
            }
            QLineSeries *series = seriesList.at(i);
            series->replace(tlist);
        }
        */
        if(allDataVector.at(0).size() < ui->spinBox->value()*datasize)
        {
            for(int i=0;i<ch;i++)
            {
                QLineSeries *series = seriesList.at(i);
                QVector<QPointF> tlist = allDataVector.at(i);
                double xValue;
                if(tlist.size())
                {
                    xValue = tlist.at(tlist.size()-1).x();
                }
                else
                {
                    xValue = 0;
                }
                for(int j=0;j<datasize;j++)
                {
                    tlist.append(QPointF(qreal(j)*(qreal)dt/1000000.0 + xValue,(double)(qToBigEndian(array[j*ch + i]))*0.305/1000.0));
                }
                allDataVector.replace(i,tlist);
                if(iCount%iRange == 0)
                {
                    axisX->setMin(tlist.at(0).x());
                    axisX->setMax(tlist.at(tlist.size()-1).x());
                    series->replace(tlist);
                }
            }

        }
        else
        {
            for(int i=0;i<ch;i++)
            {
                QLineSeries *series = seriesList.at(i);
                QVector<QPointF> tlist = allDataVector.at(i);
                double xValue = tlist.at(tlist.size()-1).x();
                // remove and append method
                tlist.remove(0,datasize);

                for(int j=0;j<datasize;j++)
                {
                    tlist.append(QPointF(qreal(j)*(qreal)dt/1000000.0 + xValue,(double)(qToBigEndian(array[j*ch + i]))*0.305/1000.0));
                }
                // replace method
                /*
                for(int j=0;j<tlist.size()-datasize;j++)
                {
                    tlist.replace(j,tlist.at(datasize+j));
                }
                for(int j=0;j<datasize;j++)
                {
                    tlist.replace(tlist.size()-datasize + j, QPointF(qreal(j)*(qreal)dt/1000000.0 + xValue,(double)(qToBigEndian(array[j*ch + i]))*0.305/1000.0));
                }*/
                allDataVector.replace(i,tlist);
                if(iCount%iRange == 0 && true)
                {
                    axisX->setMin(tlist.at(0).x());
                    axisX->setMax(tlist.at(tlist.size()-1).x());
                    series->replace(tlist);
                }

            }
        }
        iCount++;
    }
//    ui->label_12->setText(tr("Elapse : %1,iCount : %2").arg(time.elapsed()).arg(iCount%20));
}
void IONetDialog::readConfig()
{
    if(QFile::exists("Config.ini"))
    {
        QSettings setting("Config.ini",QSettings::IniFormat);
        setting.beginGroup("Params");
        setting.setIniCodec(QTextCodec::codecForName("GBK"));
        ui->lineEdit_PCIP->setText(setting.value("PCIP").toString());
        ui->spinBox_PCPort->setValue(setting.value("PCPort").toInt());
        ui->lineEdit_boartIP->setText(setting.value("BoardIP").toString());
        ui->spinBox_BoardPort->setValue(setting.value("BoardPort").toInt());
        setting.endGroup();
    }
}
