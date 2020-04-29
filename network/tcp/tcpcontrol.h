#ifndef TCPCONTROL_H
#define TCPCONTROL_H

#include <QtNetwork>
#include <QObject>
#include <QMessageBox>
#include "stdlib.h"

#include "network/protobuf/controls.pb.h"

class TCPControl: public QObject
{
    Q_OBJECT

    AmurControls *amurControls;

    std::string *serializedControls;

    QString *hostName;

    QTcpServer *tcpServer = nullptr;
    QNetworkSession *networkSession = nullptr;

public:
    TCPControl(AmurControls *controls, QString *hostname);
    ~TCPControl();

    void stop();

private slots:
    void sessionOpened();
    void sendControl();

private:
    void initNetwork();
    void protoInit();
    void protoSerialize();
};

#endif // TCPCONTROL_H
