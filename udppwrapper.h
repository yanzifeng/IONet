#ifndef UDPPWRAPPER_H
#define UDPPWRAPPER_H

#include <QObject>
#include <QUdpSocket>

class UdpPWrapper : public QObject
{
    Q_OBJECT
public:
    explicit UdpPWrapper(QObject *parent = nullptr);
    UdpPWrapper(int p,QObject *parent = nullptr);
private:
    void initialize();
private slots:
    void readPendingDiagrams();

signals:
    void sendAcqData(QByteArray);
public slots:
    void setNetParams(QStringList list, QHostAddress address, quint16 port);
    void setAcqParams(int rate,QStringList list, QHostAddress address, quint16 port);
    void startAcq(QHostAddress address, quint16 port);
    void stopAcq(QHostAddress address, quint16 port);
    void setPort(int p);
private:
    QUdpSocket *udpsocket;
    int port;
};

#endif // UDPPWRAPPER_H
