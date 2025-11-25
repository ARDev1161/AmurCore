#ifndef NETWORKFACTORY_H
#define NETWORKFACTORY_H

#include <memory>
#include "client.h"
#include "server.h"

class NetworkFactory
{
public:
    static std::shared_ptr<grpcClient> createClient(const std::string &address,
                                                    std::shared_ptr<Controls> controls,
                                                    std::shared_ptr<Sensors> sensors,
                                                    std::shared_ptr<map_service::GetMapResponse> map)
    {
        return std::make_shared<grpcClient>(grpc::CreateChannel(address, grpc::InsecureChannelCredentials()),
                                            controls, sensors, map);
    }

    static std::shared_ptr<grpcServer> createServer()
    {
        return std::make_shared<grpcServer>();
    }
};

#endif // NETWORKFACTORY_H
