#ifndef GSERVER_H
#define GSERVER_H

#include <iostream>
#include <functional>

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

  std::mutex muServer;
  std::mutex muMap;
  std::function<void()> sensors_updated_cb_;

  grpc::Status DataExchange([[maybe_unused]] grpc::ServerContext* context,
                            const Sensors* request, Controls* reply) override;

  grpc::Status DataStreamExchange([[maybe_unused]] grpc::ServerContext* context,
                                  grpc::ServerReaderWriter<Controls, Sensors>* stream) override;

  grpc::Status MapStream([[maybe_unused]] grpc::ServerContext* context,
                            grpc::ServerReaderWriter<map_service::GetMapRequest, map_service::GetMapResponse>* stream) override;
  grpc::Status PoseStream([[maybe_unused]] grpc::ServerContext* context,
                          grpc::ServerReaderWriter<map_service::PoseRequest, map_service::PoseState>* stream) override;
  grpc::Status ZoneStream([[maybe_unused]] grpc::ServerContext* context,
                          grpc::ServerReaderWriter<map_service::ZoneRequest, map_service::ZoneState>* stream) override;

public:
    void setProtosPointers(std::shared_ptr<Controls> controlsPtr,
                           std::shared_ptr<Sensors> sensorsPtr,
                           std::shared_ptr<map_service::GetMapResponse> mapPtr);
    void setSensorsUpdatedCallback(std::function<void()> cb) { sensors_updated_cb_ = std::move(cb); }
    int checkConn();

    std::mutex& getMutex() { return muServer; }
    std::mutex& getMapMutex() { return muMap; }
};

#endif // GSERVER_H
