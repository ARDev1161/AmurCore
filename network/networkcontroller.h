#ifndef NETWORKCONTROLLER_H
#define NETWORKCONTROLLER_H

#include <iostream>
#include <thread>

#include <QObject>
#include <QtNetwork>
#include <QTimer>

#include "robotentry.h"

#include "client.h"
#include "server.h"

QT_BEGIN_NAMESPACE
class QUdpSocket;
QT_END_NAMESPACE

class NetworkController : QObject
{
    Q_OBJECT

    std::shared_ptr<Controls> controls;
    std::shared_ptr<Sensors> sensors;
    std::shared_ptr<map_service::GetMapResponse> map;

    grpc::Status clientStatus;

    grpc::ServerBuilder builder;
    grpcServer service;

    int messageNo = 1;

    // Broadcast arping section
    QUdpSocket *udpSocket = nullptr;
    QList< std::shared_ptr<RobotEntry> > robots;
    QMap<quint16, QUdpSocket*> clientSockets;
public:
    NetworkController(std::shared_ptr<Controls> controlsPtr,
                      std::shared_ptr<Sensors> sensorsPtr,
                      std::shared_ptr<map_service::GetMapResponse> mapPtr);

    ~NetworkController();

    int runClient(std::string &server_address); // Server address & port for client
    int runServer(std::string &address_mask); // Address mask & port for server

    int runArpingService(int arpPort, int gRPCPort, QString &arpHeader);
    QList< std::shared_ptr<RobotEntry> > getRobots() const;
};

#endif // NETWORKCONTROLLER_H
