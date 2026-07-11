#include "server.h"

void grpcServer::notifySensorsUpdated()
{
    std::function<void()> cb;
    {
        std::lock_guard<std::mutex> lock(muSensorsUpdatedCb);
        cb = sensors_updated_cb_;
    }

    if(cb)
    {
        try
        {
            cb();
        }
        catch (const std::bad_function_call&)
        {
            // Callback was reset concurrently during shutdown.
        }
    }
}

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
  notifySensorsUpdated();

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
      notifySensorsUpdated();
    }
}

grpc::Status grpcServer::MapStream(grpc::ServerContext *context,
                                   grpc::ServerReaderWriter<map_service::GetMapRequest, map_service::GetMapResponse>* stream)
{
    map_service::GetMapRequest request;
    map_service::GetMapResponse response;
    while(true)
    {
      if(!(stream->Read(&response)))
          return grpc::Status::OK;
      {
          std::lock_guard<std::mutex> lock(muMap);
          if (map) {
              *map = response;
          }
      }
      stream->Write(request);
    }

    return grpc::Status::OK;
}

grpc::Status grpcServer::PoseStream([[maybe_unused]] grpc::ServerContext* context,
                                    grpc::ServerReaderWriter<map_service::PoseRequest, map_service::PoseState>* stream)
{
    map_service::PoseRequest request;
    map_service::PoseState state;

    while(true)
    {
      if(!(stream->Read(&state)))
          return grpc::Status::OK;
      {
          std::lock_guard<std::mutex> lock(muMap);
          if (map) {
              *map->mutable_robotpose() = state.pose();
          }
      }
      stream->Write(request);
    }

    return grpc::Status::OK;
}

grpc::Status grpcServer::ZoneStream([[maybe_unused]] grpc::ServerContext* context,
                                    grpc::ServerReaderWriter<map_service::ZoneRequest, map_service::ZoneState>* stream)
{
    map_service::ZoneRequest request;
    map_service::ZoneState state;

    while(true)
    {
      if(!(stream->Read(&state)))
          return grpc::Status::OK;
      {
          std::lock_guard<std::mutex> lock(muMap);
          if (map) {
              *map->mutable_zone_map() = state.zone_map();
          }
      }
      stream->Write(request);
    }

    return grpc::Status::OK;
}

// TODO - make async methods
// TODO - add async broadcast message stream exchange
