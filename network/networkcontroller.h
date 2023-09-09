#ifndef NETWORKCONTROLLER_H
#define NETWORKCONTROLLER_H

#include <iostream>
#include <thread>

#include <QObject>
#include <QtNetwork>
#include <QTimer>

#include "client.h"
#include "server.h"

QT_BEGIN_NAMESPACE
class QUdpSocket;
QT_END_NAMESPACE

class Robot
{
    QHostAddress m_address;
    quint16 m_port;
    quint16 portForAnswer = 0;
    QUdpSocket *m_socket;
    bool connected = false;
public:
    Robot(const QHostAddress &address, quint16 port, QUdpSocket *socket)
        : m_address(address), m_port(port), m_socket(socket)
    {}
    ~Robot(){
        delete m_socket;
    }

    void sendData(const QByteArray &data)
    {
        if(portForAnswer > 0)
            m_socket->writeDatagram(data, m_address, portForAnswer);
        else
            m_socket->writeDatagram(data, m_address, m_port);
    }

    QHostAddress address() const;
    void setAddress(const QHostAddress &newAddress);

    quint16 port() const;
    void setPort(quint16 newPort);
    quint16 getPortForAnswer() const;
    void setPortForAnswer(quint16 newPortForAnswer);
    bool isConnected() const;
};

class NetworkController : QObject
{
    Q_OBJECT

    // TODO move to Robot
    AMUR::AmurSensors *sensors;
    AMUR::AmurControls *controls;

    grpc::Status clientStatus;

    grpcServer service;

    QTimer timer;
    int messageNo = 1;

    // Broadcast arping section
    QUdpSocket *udpSocket = nullptr;
    QList<Robot*> robots;
    QMap<quint16, QUdpSocket*> clientSockets;
    int arpPort = 7776; // Port for sending server IP address to clients

    int msBroadcastDelay = 1000;
public:
    NetworkController(AMUR::AmurControls* const controls, AMUR::AmurSensors* const sensors);
    ~NetworkController();

    int runClient(std::string &server_address); // Server address & port for client
    int runServer(std::string &address_mask); // Address mask & port for server

    int runArpingService(int arpPort, int gRPCPort);

    void startBroadcasting();
    void stopBroadcasting();

};

#endif // NETWORKCONTROLLER_H
