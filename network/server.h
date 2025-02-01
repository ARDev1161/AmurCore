#ifndef GSERVER_H
#define GSERVER_H

#include <iostream>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include "network/protobuf/robot.grpc.pb.h"

using namespace Robot;

// Logic and data behind the server's behavior.
class grpcServer final : public ClientOnRobot::Service
{
  std::shared_ptr<Controls> controls;
  std::shared_ptr<Sensors> sensors;
  std::shared_ptr<map_service::GetMapResponse> map;
  std::mutex muServer, muMap;

  grpc::Status DataExchange([[maybe_unused]] grpc::ServerContext* context,
                            const Sensors* request, Controls* reply) override;

  grpc::Status DataStreamExchange([[maybe_unused]] grpc::ServerContext* context,
                                  grpc::ServerReaderWriter<Controls, Sensors>* stream) override;

  grpc::Status MapStream([[maybe_unused]] grpc::ServerContext* context,
                            grpc::ServerReaderWriter<map_service::GetMapRequest, map_service::GetMapResponse>* stream) override;

public:
    void setProtosPointers(std::shared_ptr<Controls> controlsPtr,
                           std::shared_ptr<Sensors> sensorsPtr,
                           std::shared_ptr<map_service::GetMapResponse> mapPtr);
    int checkConn();
};

#endif // GSERVER_H
