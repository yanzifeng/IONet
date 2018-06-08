#include "udppwrapper.h"
#include <QThread>
#include <QDebug>

UdpPWrapper::UdpPWrapper(QObject *parent) : QObject(parent)
{
    qDebug() << "Construction thread:" << QThread::currentThreadId();
    initialize();
}
UdpPWrapper::UdpPWrapper(int p,QObject *parent) : QObject(parent)
{
    qDebug() << "Construction thread:" << QThread::currentThreadId();
    port = p;
    initialize();
}
void UdpPWrapper::initialize()
{
    udpsocket = new QUdpSocket();
    qDebug() << udpsocket->bind(QHostAddress::Any,port);
    connect(udpsocket,SIGNAL(readyRead()),this,SLOT(readPendingDiagrams()));

    qDebug() << "Waiting for UDP data from port " << port << " ... \n";
}
void UdpPWrapper::setAcqParams(int rate,QStringList list,QHostAddress address, quint16 port)
{
    QByteArray ba;
    ba.resize(4);
    ba[0] = 0xAB;
    ba[1] = 0xC3;
    ba[2] = 0x00;
    ba[3] = rate;
    udpsocket->writeDatagram(ba,address,port); //set rate
    ba[0] = 0xAB;
    ba[1] = 0xC4;
    QString str;
    for(int i=0;i<16;i++)
    {
        if(list.contains(QString::number(i)))
        {
            str.insert(0,"1");
        }
        else
        {
            str.insert(0,"0");
        }
    }
    ba[2] = str.left(8).toInt(0,2);
    ba[3] = str.right(8).toInt(0,2);
    qint64 ret = udpsocket->writeDatagram(ba,address,port); //set channels
    qDebug() <<"set acq params : " << address.toString() << port << ret;
}
void UdpPWrapper::setNetParams(QStringList list,QHostAddress address, quint16 port)
{
    qDebug() << list;
    QByteArray ba;
    ba.resize(14);
    ba[0] = 0xAB;
    ba[1] = 0xC1;

    QString boardip = list.at(2);
    QStringList boardipList = boardip.split(".");
    for(int i=0;i<4;i++)
    {
        ba[i+8] = boardipList.at(i).toInt();
    }
    QString pcip = list.at(0);
    QStringList pcipList = pcip.split(".");
    qDebug() << pcipList;
    for(int i=0;i<4;i++)
    {
        ba[i+2] = pcipList.at(i).toInt();
    }

    int pcport = list.at(1).toInt();
    ba[7] = pcport & 0x00FF;
    ba[6] = (pcport & 0xFF00) >> 8;
    int boardport = list.at(3).toInt();
    ba[13] = boardport & 0x00FF;
    ba[12] = (boardport & 0xFF00) >> 8;

    qint64 ret = udpsocket->writeDatagram(ba,address,port);
    qDebug() <<"set net params : " << address.toString() << port << ret;
}
void UdpPWrapper::startAcq(QHostAddress address, quint16 port)
{
    QByteArray ba;
    ba.resize(4);
    ba[0] = 0xAB;
    ba[1] = 0xD1;
    ba[2] = 0x00;
    ba[3] = 0xAA;
    qint64 ret = udpsocket->writeDatagram(ba,address,port);
    qDebug() <<"start acq : " << address.toString() << port << ret;
}
void UdpPWrapper::stopAcq(QHostAddress address, quint16 port)
{
    QByteArray ba;
    ba.resize(4);
    ba[0] = 0xAB;
    ba[1] = 0xD1;
    ba[2] = 0x00;
    ba[3] = 0xBB;
    qint64 ret = udpsocket->writeDatagram(ba,address,port);
    qDebug() <<"stop acq";
}
void UdpPWrapper::readPendingDiagrams()
{
//    qDebug() << "Reading thread:" << QThread::currentThreadId();

    while(udpsocket->waitForReadyRead())
    {
        QByteArray datagram;
        datagram.resize(udpsocket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        udpsocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
//        qDebug() << datagram.size() << " bytes received .... \n";
        sendAcqData(datagram);
    }
}
void UdpPWrapper::setPort(int p)
{
    port = p;
    udpsocket->bind(port);
}
