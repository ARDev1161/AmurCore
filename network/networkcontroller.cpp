#include "networkcontroller.h"


NetworkController::NetworkController(AMUR::AmurControls* const controls, AMUR::AmurSensors* const sensors)  // TODO - add robot id & &<vector> of robots id
{
    this->controls = controls;
    this->sensors = sensors;

    udpSocket = new QUdpSocket(this);

//    connect(&timer, &QTimer::timeout, this, &NetworkController::arpBroadcastMessage);
    // TODO
}

NetworkController::~NetworkController()
{
    // Clear list of robots
    for (auto robot : robots) {
        delete robot;
    }
    robots.clear();
}

int NetworkController::runClient(std::string &server_address) // TODO - send const &<vector> of robots id
{
    // Instantiate the client. It requires a channel, out of which the actual RPCs
    // are created. This channel models a connection to an endpoint specified by
    // the argument "--target=" which is the only expected argument.
    // We indicate that the channel isn't authenticated (use of
    // InsecureChannelCredentials()).

    std::thread thr([&]()
     {
        grpcClient client(grpc::CreateChannel(
            server_address, grpc::InsecureChannelCredentials()), sensors, controls);

        clientStatus = client.DataStreamExchange();
        std::cout << "State is OK?: " << clientStatus.ok() << std::endl;
     }
    );
    thr.detach();

    return 0;
}

int NetworkController::runServer(std::string &address_mask) // TODO - send const &<vector> of robots id
{
    std::thread thr([&]()
     {
      grpc::EnableDefaultHealthCheckService(true);
      grpc::reflection::InitProtoReflectionServerBuilderPlugin();

      // Send protos pointers to server
      service.setProtosPointers(controls, sensors);

      // Listen on the given address without any authentication mechanism.
      builder.AddListeningPort(address_mask, grpc::InsecureServerCredentials());

      // Register "service" as the instance through which we'll communicate with
      // clients. In this case it corresponds to an *synchronous* service.
      builder.RegisterService(&service);

      // Finally assemble the server.
      std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
      std::cout << "Server listening on " << address_mask << std::endl;

      // Wait for the server to shutdown. Note that some other thread must be
      // responsible for shutting down the server for this call to ever return.
      server->Wait();

      std::cout << "Server stopped " << std::endl;
     }
    );
    thr.detach();

    return 0;
}

void NetworkController::startBroadcasting()
{
    timer.start(msBroadcastDelay);
}

void NetworkController::stopBroadcasting()
{
    if(timer.isActive())
        timer.stop();
}

int NetworkController::runArpingService(int arpPort, int gRPCPort)
{
    udpSocket->bind(QHostAddress::AnyIPv4, arpPort);

    QObject::connect(udpSocket, &QUdpSocket::readyRead, [&, gRPCPort](){
        while (udpSocket->hasPendingDatagrams()) {
            QByteArray datagram;
            datagram.resize(udpSocket->pendingDatagramSize());

            QHostAddress senderAddress;
            quint16 senderBCastPort;
            udpSocket->readDatagram(datagram.data(), datagram.size(), &senderAddress, &senderBCastPort);

            // Split message by separator
            QString message = QString::fromUtf8(datagram);
            QStringList parsed = message.split(u':');

            // Process message if it have preambula - "AMUR:"
            if(parsed.empty())
                continue;

            if("AMUR" == parsed.at(0)){
                qDebug() << "Received arping message: " << message << " from " << senderAddress.toString() << ":" << senderBCastPort;

                int arpingPort = 0;
                if(parsed.size() > 2)
                    arpingPort = parsed.at(2).toInt();

                // Check if the client is already in the list
                Robot *robot = nullptr;
                for (int i = 0; i < robots.size(); ++i) {
                    if (robots[i]->address() == senderAddress && robots[i]->port() == senderBCastPort) {
                        robot = robots[i];
                        break;
                    }
                }

                // Add a new client if it not found in list of robots
                if (!robot) {
                    robot = new Robot(senderAddress, senderBCastPort, udpSocket);
                    if(arpingPort > 0)
                        robot->setPortForAnswer(arpingPort);
                    robots.append(robot);
                    qDebug() << "Added new client " << senderAddress.toString() << ":" << senderBCastPort;
                }

                QByteArray response = "OK:" + QByteArray::number(gRPCPort);
                robot->sendData(response);
            }

        }
    });

    std::cout << "" << std::endl;
    return 0;
}


quint16 Robot::getPortForAnswer() const
{
    return portForAnswer;
}

void Robot::setPortForAnswer(quint16 newPortForAnswer)
{
    portForAnswer = newPortForAnswer;
}

QHostAddress Robot::address() const
{
    return m_address;
}

void Robot::setAddress(const QHostAddress &newAddress)
{
    m_address = newAddress;
}

quint16 Robot::port() const
{
    return m_port;
}

void Robot::setPort(quint16 newPort)
{
    m_port = newPort;
}
