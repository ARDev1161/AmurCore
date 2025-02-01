#ifndef GCLIENT_H
#define GCLIENT_H

#include <iostream>
#include <thread>
#include <chrono>

#include <grpcpp/grpcpp.h>

#include "network/protobuf/robot.grpc.pb.h"

using namespace Robot;

class grpcClient
{
    std::unique_ptr<ServerOnRobot::Stub> stub_;
    std::shared_ptr<grpc::Channel> clientChannel;

    std::shared_ptr<Controls> controls;
    std::shared_ptr<Sensors> sensors;
    std::shared_ptr<map_service::GetMapResponse> map;

    bool stoppedStream = true;

    std::mutex muClient;

 public:
    grpcClient(std::shared_ptr<grpc::Channel> channel,
               std::shared_ptr<Controls> controlsPtr,
               std::shared_ptr<Sensors> sensorsPtr,
               std::shared_ptr<map_service::GetMapResponse> mapPtr);

    grpc::Status DataExchange();
    grpc::Status DataStreamExchange();
    grpc::Status MapStream();
    void stopStream();
};

#endif // GCLIENT_H
