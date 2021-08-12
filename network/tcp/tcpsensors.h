#ifndef TCPSENSORS_H
#define TCPSENSORS_H

#include <QtNetwork>
#include <QObject>

#include "sensors/sensors.h"
#include "network/protobuf/amur.grpc.pb.h"

class TCPSensors: public QObject
{
    Q_OBJECT

    AMUR::AmurSensors *sensors;

    QNetworkSession *networkSession = nullptr;
    QDataStream in;
    QTcpSocket *tcpSocket = nullptr;
    QString currentSensors;
    QString *hostName;

    int port = 7777;

public:
    TCPSensors(AMUR::AmurSensors *sensors, QString *hostname);
    ~TCPSensors();

    void stop();

private slots:
    void requestNewControl();
    void readControl();
    void displayError(QAbstractSocket::SocketError socketError);
    void sessionOpened();

private:
    void initNetwork();

};

#endif // TCPSENSORS_H
