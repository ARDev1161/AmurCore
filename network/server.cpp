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
    {
        std::lock_guard<std::mutex> control_lock(muServer);
        controls = std::move(controlsPtr);
        sensors = std::move(sensorsPtr);
    }
    {
        std::lock_guard<std::mutex> map_lock(muMap);
        map = std::move(mapPtr);
    }
}

grpc::Status grpcServer::DataExchange (grpc::ServerContext* context,
                          const Sensors* request, Controls* reply)
{
  std::unique_lock<std::mutex> ul(muServer);
  if (context->IsCancelled())
      return grpc::Status(grpc::StatusCode::CANCELLED, "client disconnected");
  if (!sensors || !controls)
      return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "transport state unavailable");
  *sensors = *request;
  *reply = *controls;
  // The generic Controls exchange is not an authenticated v2 velocity path.
  reply->mutable_basecontrol()->set_linearvelocity(0.0);
  reply->mutable_basecontrol()->set_angularvelocity(0.0);
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
            if (context->IsCancelled())
                return grpc::Status(grpc::StatusCode::CANCELLED, "client disconnected");
            if (!sensors || !controls)
                return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "transport state unavailable");
            *sensors = incoming;
            outgoing = *controls;
            // Never leak stale/unleased motion from any legacy GUI writer.
            outgoing.mutable_basecontrol()->set_linearvelocity(0.0);
            outgoing.mutable_basecontrol()->set_angularvelocity(0.0);
        }
        notifySensorsUpdated();
        if (!stream->Write(outgoing)) {
            return context->IsCancelled()
                ? grpc::Status(grpc::StatusCode::CANCELLED, "client disconnected")
                : grpc::Status(grpc::StatusCode::UNAVAILABLE, "control stream write failed");
        }
    }
    if (context->IsCancelled())
        return grpc::Status(grpc::StatusCode::CANCELLED, "client disconnected");
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
          return context->IsCancelled() ? grpc::Status(grpc::StatusCode::CANCELLED, "client disconnected") : grpc::Status::OK;
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
          return context->IsCancelled() ? grpc::Status(grpc::StatusCode::CANCELLED, "client disconnected") : grpc::Status::OK;
      }
    }

    return context->IsCancelled() ? grpc::Status(grpc::StatusCode::CANCELLED, "client disconnected") : grpc::Status::OK;
}

grpc::Status grpcServer::PoseStream(grpc::ServerContext* context,
                                    grpc::ServerReaderWriter<map_service::PoseRequest, map_service::PoseState>* stream)
{
    map_service::PoseRequest request;
    map_service::PoseState state;

    while(true)
    {
      if(!(stream->Read(&state)))
          return context->IsCancelled() ? grpc::Status(grpc::StatusCode::CANCELLED, "client disconnected") : grpc::Status::OK;
      {
          std::lock_guard<std::mutex> lock(muMap);
          if (map) {
              *map->mutable_robotpose() = state.pose();
          }
      }
      if (!stream->Write(request)) {
          return context->IsCancelled() ? grpc::Status(grpc::StatusCode::CANCELLED, "client disconnected") : grpc::Status::OK;
      }
    }

    return context->IsCancelled() ? grpc::Status(grpc::StatusCode::CANCELLED, "client disconnected") : grpc::Status::OK;
}

grpc::Status grpcServer::ZoneStream(grpc::ServerContext* context,
                                    grpc::ServerReaderWriter<map_service::ZoneRequest, map_service::ZoneState>* stream)
{
    map_service::ZoneRequest request;
    map_service::ZoneState state;

    while(true)
    {
      if(!(stream->Read(&state)))
          return context->IsCancelled() ? grpc::Status(grpc::StatusCode::CANCELLED, "client disconnected") : grpc::Status::OK;
      {
          std::lock_guard<std::mutex> lock(muMap);
          if (map) {
              *map->mutable_zone_map() = state.zone_map();
          }
      }
      if (!stream->Write(request)) {
          return context->IsCancelled() ? grpc::Status(grpc::StatusCode::CANCELLED, "client disconnected") : grpc::Status::OK;
      }
    }

    return context->IsCancelled() ? grpc::Status(grpc::StatusCode::CANCELLED, "client disconnected") : grpc::Status::OK;
}

// TODO - make async methods
// TODO - add async broadcast message stream exchange
