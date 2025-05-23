#include "networkcontroller.h"

NetworkController::NetworkController(std::shared_ptr<Controls> controlsPtr,
                                     std::shared_ptr<Sensors> sensorsPtr,
                                     std::shared_ptr<map_service::GetMapResponse> mapPtr)  // TODO - add robot id & &<vector> of robots id
    : controls(controlsPtr),
      sensors(sensorsPtr),
      map(mapPtr)
{
    udpSocket = new QUdpSocket(this);
    serverPtr = std::make_shared<grpcServer>();
}

NetworkController::~NetworkController()
{
    // Clear list of robots
    robots.clear();
}

int NetworkController::runClient(std::string &server_address) // TODO - send const &<vector> of robots id
{
    // Instantiate the client. It requires a channel, out of which the actual RPCs
    // are created. This channel models a connection to an endpoint specified by
    // the argument "--target=" which is the only expected argument.
    // We indicate that the channel isn't authenticated (use of
    // InsecureChannelCredentials()).

    clientPtr = std::make_shared<grpcClient>(grpc::CreateChannel(
        server_address, grpc::InsecureChannelCredentials()), controls, sensors, map);

    std::thread thr([&, this]()
     {

        clientStatus = this->clientPtr->DataStreamExchange();
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
      serverPtr->setProtosPointers(controls, sensors, map);

      // Listen on the given address without any authentication mechanism.
      builder.AddListeningPort(address_mask, grpc::InsecureServerCredentials());

      // Register "service" as the instance through which we'll communicate with
      // clients. In this case it corresponds to an *synchronous* service.
      builder.RegisterService(serverPtr.get());

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

int NetworkController::runArpingService(int arpPort, int gRPCPort, QString &arpHeader)
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

            if(arpHeader == parsed.at(0)){
                qDebug() << "Received arping message: " << message << " from " << senderAddress.toString() << ":" << senderBCastPort;

                QString machineID;
                if(parsed.size() > 1)
                    machineID = parsed.at(1);

                int arpingPort = 0;
                if(parsed.size() > 2)
                    arpingPort = parsed.at(2).toInt();

                // Check if the client is already in the list
                std::shared_ptr<RobotEntry> robot;
                for (int i = 0; i < robots.size(); ++i) {
                    if (robots[i]->address() == senderAddress) {
                        robot = robots[i];
                        break;
                    }
                }

                // Add a new client if it not found in list of robots
                if (!robot) {
                    robot = std::make_shared<RobotEntry>(udpSocket, senderAddress, senderBCastPort);
                    robot->setMachineID(machineID);

                    // set custom port for answer if setted
                    if(arpingPort > 0)
                        robot->setPort(arpingPort);

                    robots.append(robot);
                    qDebug() << "Added new client " << senderAddress.toString() << ":" << arpingPort;
                }

                QByteArray response = "OK:" + QByteArray::number(gRPCPort);
                robot->sendData(response);
            }

        }
    });

    std::cout << "" << std::endl;
    return 0;
}

QList< std::shared_ptr<RobotEntry> > NetworkController::getRobots() const
{
    return robots;
}
