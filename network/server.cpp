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
    // Network I/O must never hold the mutex also used by the operator UI.
    // Read into a local message; publish a consistent sensor/control snapshot
    // under the lock, then write the response without blocking UI access.
    Sensors incoming;
    while (stream->Read(&incoming))
    {
        Controls outgoing;
        {
            std::lock_guard<std::mutex> lock(muServer);
            *sensors = incoming;
            outgoing = *controls;
        }
        notifySensorsUpdated();
        if (!stream->Write(outgoing)) {
            break;
        }
    }
    return grpc::Status::OK;
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
              // MapStream may carry only OccupancyGrid/map_seq. Preserve
              // independently streamed pose and zones when omitted.
              if (response.has_map()) {
                  *map->mutable_map() = response.map();
              }
              map->set_map_seq(response.map_seq());
              if (response.has_robotpose()) {
                  *map->mutable_robotpose() = response.robotpose();
              }
              if (response.has_zone_map()) {
                  *map->mutable_zone_map() = response.zone_map();
                  map->set_zone_seq(response.zone_seq());
              }
          }
      }
      if (!stream->Write(request)) {
          return grpc::Status::OK;
      }
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
      if (!stream->Write(request)) {
          return grpc::Status::OK;
      }
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
      if (!stream->Write(request)) {
          return grpc::Status::OK;
      }
    }

    return grpc::Status::OK;
}

// TODO - make async methods
// TODO - add async broadcast message stream exchange
