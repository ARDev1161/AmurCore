#include <cmath>
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
        ++state_generation_;
    }
    {
        std::lock_guard<std::mutex> map_lock(muMap);
        map = std::move(mapPtr);
        ++map_generation_;
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
  reply->clear_velocity_command();
  reply->clear_request_velocity_lease();
  reply->clear_wheelmotors();
  reply->mutable_handmotors()->set_leftpower(0);
  reply->mutable_handmotors()->set_rightpower(0);
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
    std::uint64_t bound_generation;
    {
        std::lock_guard<std::mutex> lock(muServer);
        if (!controls || !sensors)
            return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "transport state unavailable");
        bound_generation = state_generation_;
    }
    Sensors incoming;
    while (stream->Read(&incoming))
    {
        Controls outgoing;
        {
            std::lock_guard<std::mutex> lock(muServer);
            if (context->IsCancelled())
                return grpc::Status(grpc::StatusCode::CANCELLED, "client disconnected");
            if (!sensors || !controls || state_generation_ != bound_generation)
                return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "transport state generation changed");
            *sensors = incoming;
            outgoing = *controls;
            // Never leak stale/unleased motion from any legacy GUI writer.
            outgoing.mutable_basecontrol()->set_linearvelocity(0.0);
            outgoing.mutable_basecontrol()->set_angularvelocity(0.0);
            const auto auth = context->auth_context();
            const auto transport = auth ? auth->FindPropertyValues("transport_security_type")
                                        : std::vector<grpc::string_ref>{};
            const bool authenticated = auth && auth->IsPeerAuthenticated() &&
                transport.size() == 1 && transport.front() == "ssl";
            if (!authenticated || !incoming.has_robot_info() ||
                !incoming.robot_info().has_base()) {
                outgoing.clear_velocity_command();
                outgoing.clear_request_velocity_lease();
            } else if (outgoing.has_velocity_command()) {
                const auto &cmd = outgoing.velocity_command();
                if (!incoming.has_velocity_lease() || !incoming.velocity_lease().granted() ||
                    cmd.robot_id() != incoming.robot_info().robot_id() ||
                    cmd.robot_id() != incoming.velocity_lease().robot_id() ||
                    cmd.session_id() != incoming.velocity_lease().session_id() ||
                    cmd.lease_id() != incoming.velocity_lease().lease_id()) {
                    outgoing.clear_velocity_command();
                }
            }
            outgoing.clear_wheelmotors();
            outgoing.mutable_handmotors()->set_leftpower(0);
            outgoing.mutable_handmotors()->set_rightpower(0);
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
    {
        std::lock_guard<std::mutex> lock(muServer);
        if (!controls || !sensors || state_generation_ != bound_generation)
            return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "transport state generation changed");
    }
    return grpc::Status::OK;
}

grpc::Status grpcServer::MapStream(grpc::ServerContext *context,
                                   grpc::ServerReaderWriter<map_service::GetMapRequest, map_service::GetMapResponse>* stream)
{
    map_service::GetMapRequest request;
    map_service::GetMapResponse response;
    std::uint64_t bound_map_generation;
    {
        std::lock_guard<std::mutex> lock(muMap);
        if (!map)
            return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "map state unavailable");
        bound_map_generation = map_generation_;
    }
    while(true)
    {
      if(!(stream->Read(&response))) {
          if (context->IsCancelled()) return grpc::Status(grpc::StatusCode::CANCELLED, "client disconnected");
          std::lock_guard<std::mutex> lock(muMap);
          return (!map || map_generation_ != bound_map_generation)
              ? grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "map state generation changed") : grpc::Status::OK;
      }
      {
          std::lock_guard<std::mutex> lock(muMap);
          if (!map || map_generation_ != bound_map_generation)
              return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "map state generation changed");
          if (map) {
              // MapStream may carry only OccupancyGrid/map_seq. Preserve
              // independently streamed pose and zones when omitted.
              if (response.has_map() && ((response.map().width() == 0 && response.map().height() == 0 && response.map().data().empty()) ||
                   (response.map().width() > 0 && response.map().height() > 0 &&
                    std::isfinite(response.map().resolution()) && response.map().resolution() > 0.0f &&
                    response.map().data().size() ==
                        static_cast<std::uint64_t>(response.map().width()) * response.map().height()))) {
                  *map->mutable_map() = response.map();
                  map->set_map_seq(response.map_seq());
              }
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
          return context->IsCancelled() ? grpc::Status(grpc::StatusCode::CANCELLED, "client disconnected")
                                        : grpc::Status(grpc::StatusCode::UNAVAILABLE, "map/pose/zone stream write failed");
      }
    }

    return context->IsCancelled() ? grpc::Status(grpc::StatusCode::CANCELLED, "client disconnected") : grpc::Status::OK;
}

grpc::Status grpcServer::PoseStream(grpc::ServerContext* context,
                                    grpc::ServerReaderWriter<map_service::PoseRequest, map_service::PoseState>* stream)
{
    map_service::PoseRequest request;
    map_service::PoseState state;

    std::uint64_t bound_map_generation;
    {
        std::lock_guard<std::mutex> lock(muMap);
        if (!map)
            return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "map state unavailable");
        bound_map_generation = map_generation_;
    }
    while(true)
    {
      if(!(stream->Read(&state))) {
          if (context->IsCancelled()) return grpc::Status(grpc::StatusCode::CANCELLED, "client disconnected");
          std::lock_guard<std::mutex> lock(muMap);
          return (!map || map_generation_ != bound_map_generation)
              ? grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "map state generation changed") : grpc::Status::OK;
      }
      {
          std::lock_guard<std::mutex> lock(muMap);
          if (!map || map_generation_ != bound_map_generation)
              return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "map state generation changed");
          // A stream without a pose is not an odometry update. In particular,
          // do not replace the last valid pose with protobuf's default origin.
          if (map && state.has_pose()) {
              *map->mutable_robotpose() = state.pose();
          }
      }
      if (!stream->Write(request)) {
          return context->IsCancelled() ? grpc::Status(grpc::StatusCode::CANCELLED, "client disconnected")
                                        : grpc::Status(grpc::StatusCode::UNAVAILABLE, "map/pose/zone stream write failed");
      }
    }

    return context->IsCancelled() ? grpc::Status(grpc::StatusCode::CANCELLED, "client disconnected") : grpc::Status::OK;
}

grpc::Status grpcServer::ZoneStream(grpc::ServerContext* context,
                                    grpc::ServerReaderWriter<map_service::ZoneRequest, map_service::ZoneState>* stream)
{
    map_service::ZoneRequest request;
    map_service::ZoneState state;

    std::uint64_t bound_map_generation;
    {
        std::lock_guard<std::mutex> lock(muMap);
        if (!map)
            return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "map state unavailable");
        bound_map_generation = map_generation_;
    }
    while(true)
    {
      if(!(stream->Read(&state))) {
          if (context->IsCancelled()) return grpc::Status(grpc::StatusCode::CANCELLED, "client disconnected");
          std::lock_guard<std::mutex> lock(muMap);
          return (!map || map_generation_ != bound_map_generation)
              ? grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "map state generation changed") : grpc::Status::OK;
      }
      {
          std::lock_guard<std::mutex> lock(muMap);
          if (!map || map_generation_ != bound_map_generation)
              return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "map state generation changed");
          // An absent ZoneMap is not a request to clear the last valid
          // zones. Explicitly present empty ZoneMap still removes all zones.
          if (map && state.has_zone_map()) {
              *map->mutable_zone_map() = state.zone_map();
              map->set_zone_seq(state.zone_seq());
          }
      }
      if (!stream->Write(request)) {
          return context->IsCancelled() ? grpc::Status(grpc::StatusCode::CANCELLED, "client disconnected")
                                        : grpc::Status(grpc::StatusCode::UNAVAILABLE, "map/pose/zone stream write failed");
      }
    }

    return context->IsCancelled() ? grpc::Status(grpc::StatusCode::CANCELLED, "client disconnected") : grpc::Status::OK;
}

// TODO - make async methods
// TODO - add async broadcast message stream exchange
