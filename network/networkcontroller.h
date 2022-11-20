#ifndef NETWORKCONTROLLER_H
#define NETWORKCONTROLLER_H

#include <iostream>
#include <thread>

#include <QtNetwork>
#include <QTimer>
#include <QObject>

#include "client.h"
#include "server.h"
#include "jsonworker.h"

QT_BEGIN_NAMESPACE
class QUdpSocket;
QT_END_NAMESPACE

class NetworkController : QObject
{
    Q_OBJECT

    AMUR::AmurSensors *sensors;
    AMUR::AmurControls *controls;

    grpc::Status clientStatus;

    grpc::ServerBuilder builder;
    grpcServer service;

    JSONWorker *json = nullptr;

    // Broadcasting section
    QUdpSocket *udpSocket = nullptr;
    QTimer timer;
    int messageNo = 1;
    int msBroadcastDelay = 1000;
    int arpPort = 11111; // Port for sending server IP address to clients

private slots:
    int arpBroadcastMessage(std::string &broadcast_address); // Sending a package to be detected by robots

public:
    NetworkController(AMUR::AmurControls* const controls, AMUR::AmurSensors* const sensors);

    int runClient(std::string &server_address); // Server address & port for client
    int runServer(std::string &address_mask); // Address mask & port for server

    void startBroadcasting();
    void stopBroadcasting();

};

#endif // NETWORKCONTROLLER_H
