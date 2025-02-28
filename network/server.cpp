#include "server.h"

void grpcServer::setProtosPointers( std::shared_ptr<Controls> controlsPtr,
                                    std::shared_ptr<Sensors> sensorsPtr,
                                    std::shared_ptr<map_service::GetMapResponse> mapPtr)
{
    this->controls = controlsPtr;
    this->sensors = sensorsPtr;
    this->map = mapPtr;
}

grpc::Status grpcServer::DataExchange ([[maybe_unused]] grpc::ServerContext* context,
                          const Sensors* request, Controls* reply)
{
  std::unique_lock<std::mutex> ul(muServer);
  *sensors = *request;
  *reply = *controls;
  ul.unlock();

  return grpc::Status::OK;
}

grpc::Status grpcServer::DataStreamExchange ([[maybe_unused]] grpc::ServerContext* context,
                                grpc::ServerReaderWriter<Controls, Sensors >* stream)
{
    std::unique_lock<std::mutex> ul(muServer, std::defer_lock);

    while(true)
    {
      ul.lock();
      if(!(stream->Read(sensors.get())))
          return grpc::Status::OK;
      // Write controls
      stream->Write(*controls);
      ul.unlock();
    }
}

grpc::Status grpcServer::MapStream(grpc::ServerContext *context,
                                   grpc::ServerReaderWriter<map_service::GetMapRequest, map_service::GetMapResponse>* stream)
{
    map_service::GetMapRequest request;

    std::unique_lock<std::mutex> ul(muMap, std::defer_lock);
    while(true)
    {
      ul.lock();
      if(!(stream->Read(map.get())))
          return grpc::Status::OK;

      // Write controls
      stream->Write(request);
      ul.unlock();
    }

    return grpc::Status::OK;
}

// TODO - make async methods
// TODO - add async broadcast message stream exchange
