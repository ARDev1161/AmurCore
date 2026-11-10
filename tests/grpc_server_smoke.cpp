// Headless gRPC integration test: real AmurCore service, no ROS2 or Qt event loop.
// Covers legacy unary RPC, v2 descriptor on the same wire, streamed controls,
// idle-stream mutex liveness and map update. No physical robot is required.
#include "network/server.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>

namespace {
void require(bool value, const char *description) {
    if (!value) {
        throw std::runtime_error(description);
    }
}
}  // namespace

int main() {
    auto controls = std::make_shared<Robot::Controls>();
    auto sensors = std::make_shared<Robot::Sensors>();
    auto map = std::make_shared<map_service::GetMapResponse>();

    grpcServer service;
    service.setProtosPointers(controls, sensors, map);
    std::atomic<int> notifications{0};
    service.setSensorsUpdatedCallback([&notifications]() { ++notifications; });

    {
        std::lock_guard<std::mutex> lock(service.getMutex());
        controls->mutable_basecontrol()->set_linearvelocity(17.5);
        controls->mutable_basecontrol()->set_angularvelocity(-3.0);
    }

    grpc::ServerBuilder builder;
    int port = 0;
    builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &port);
    builder.RegisterService(&service);
    auto server = builder.BuildAndStart();
    require(server != nullptr && port != 0, "Cannot start loopback gRPC server");
    auto channel = grpc::CreateChannel("127.0.0.1:" + std::to_string(port),
                                       grpc::InsecureChannelCredentials());
    auto stub = Robot::ClientOnRobot::NewStub(channel);
    try {
        // Legacy sensor telemetry contains no v2 descriptor; existing controls still work.
        {
            Robot::Sensors legacy;
            legacy.mutable_batterystate()->set_percentage(0.5);
            Robot::Controls reply;
            grpc::ClientContext context;
            require(stub->DataExchange(&context, legacy, &reply).ok(),
                    "Legacy unary DataExchange failed");
            require(reply.basecontrol().linearvelocity() == 17.5,
                    "Legacy drive control changed");
            std::lock_guard<std::mutex> lock(service.getMutex());
            require(!sensors->has_robot_info(), "Legacy payload invented robot metadata");
        }

        Robot::Sensors modern;
        modern.mutable_batterystate()->set_percentage(0.9);
        auto *info = modern.mutable_robot_info();
        info->set_robot_id("unit-robot");
        info->set_display_name("Test robot");
        info->set_api_major(2);
        auto *cap = info->add_capabilities();
        cap->set_id("manual.velocity");
        cap->add_velocity_axes("linear.x");
        cap->add_velocity_axes("angular.z");

        {
            Robot::Controls reply;
            grpc::ClientContext context;
            require(stub->DataExchange(&context, modern, &reply).ok(),
                    "New unary DataExchange failed");
            std::lock_guard<std::mutex> lock(service.getMutex());
            require(sensors->robot_info().robot_id() == "unit-robot",
                    "Robot ID was not retained by workstation");
            require(sensors->robot_info().capabilities(0).velocity_axes_size() == 2,
                    "Velocity axes were not retained");
        }

        {
            grpc::ClientContext context;
            context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));
            auto stream = stub->DataStreamExchange(&context);
            require(stream && stream->Write(modern), "Could not send v2 stream snapshot");
            Robot::Controls reply;
            require(stream->Read(&reply), "Could not read streamed controls");
            require(reply.basecontrol().angularvelocity() == -3.0,
                    "Streaming controls payload changed");

            // A legacy peer may reuse an established stream after a v2
            // snapshot. Each Sensors frame is a complete snapshot: absent
            // descriptor and battery fields must not retain stale values.
            Robot::Sensors legacy_on_stream;
            require(stream->Write(legacy_on_stream),
                    "Could not send legacy snapshot on active v2 stream");
            require(stream->Read(&reply),
                    "No controls reply to legacy snapshot on v2 stream");
            {
                std::lock_guard<std::mutex> lock(service.getMutex());
                require(!sensors->has_robot_info(),
                        "Legacy stream snapshot retained stale v2 descriptor");
                require(!sensors->has_batterystate(),
                        "Legacy stream snapshot retained stale battery");
            }
            Robot::Sensors zero_battery;
            zero_battery.mutable_robot_info()->set_robot_id("unit-robot");
            zero_battery.mutable_batterystate()->set_present(true);
            zero_battery.mutable_batterystate()->set_percentage(0.0f);
            require(stream->Write(zero_battery),
                    "Could not restore v2 snapshot on active stream");
            require(stream->Read(&reply),
                    "No controls reply to restored v2 snapshot");
            {
                std::lock_guard<std::mutex> lock(service.getMutex());
                require(sensors->robot_info().robot_id() == "unit-robot",
                        "Restored stream snapshot lost robot identity");
                require(sensors->has_batterystate() && sensors->batterystate().present(),
                        "Zero-percent battery was confused with missing battery");
                require(sensors->batterystate().percentage() == 0.0f,
                        "Zero-percent battery value was not preserved");
            }

            // Give the server time to enter its next blocking Read. Otherwise
            // an old implementation could briefly release the lock and make a
            // race-dependent false positive in this regression test.
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
            // GUI/control mutex must remain available while the peer is idle.
            bool unlocked_during_idle = false;
            for (int i = 0; i != 40; ++i) {
                if (service.getMutex().try_lock()) {
                    service.getMutex().unlock();
                    unlocked_during_idle = true;
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            context.TryCancel();
            stream->Finish();
            require(unlocked_during_idle, "Idle stream blocks workstation mutex");
            require(notifications.load() > 0, "Stream did not notify observer");
        }

        // The same workstation session can transition from a v2 peer to a
        // legacy peer. Unknown-field fallback must never leave the previous
        // robot's capabilities visible in the next Sensors snapshot.
        {
            Robot::Sensors legacy_after_v2;
            legacy_after_v2.mutable_batterystate()->set_percentage(0.25);
            Robot::Controls reply;
            grpc::ClientContext context;
            require(stub->DataExchange(&context, legacy_after_v2, &reply).ok(),
                    "Legacy unary reconnect after v2 stream failed");
            require(reply.basecontrol().linearvelocity() == 17.5,
                    "Legacy command reply changed after v2 stream");
            std::lock_guard<std::mutex> lock(service.getMutex());
            require(!sensors->has_robot_info(),
                    "Legacy reconnect retained stale v2 descriptor");
            require(sensors->batterystate().percentage() == 0.25,
                    "Legacy sensor snapshot was not applied");
        }

        // Battery expiry is encoded as field absence, not 0%. A new
        // complete Sensors snapshot must clear the previous battery and
        // subsequently accept a recovered sample without leaking state.
        {
            Robot::Sensors without_battery;
            without_battery.mutable_robot_info()->set_robot_id("unit-robot");
            Robot::Controls reply;
            grpc::ClientContext context;
            require(stub->DataExchange(&context, without_battery, &reply).ok(),
                    "Battery-absent unary snapshot failed");
            std::lock_guard<std::mutex> lock(service.getMutex());
            require(!sensors->has_batterystate(),
                    "Battery field survived a complete snapshot without battery");
            require(sensors->has_robot_info(),
                    "Battery expiry incorrectly removed robot identity");
        }
        {
            Robot::Sensors recovered;
            recovered.mutable_robot_info()->set_robot_id("unit-robot");
            auto *pack = recovered.mutable_batterystate();
            pack->set_present(true);
            pack->set_percentage(0.76f);
            pack->set_power_supply_status(Power::BatteryState::CHARGING);
            Robot::Controls reply;
            grpc::ClientContext context;
            require(stub->DataExchange(&context, recovered, &reply).ok(),
                    "Recovered battery unary snapshot failed");
            std::lock_guard<std::mutex> lock(service.getMutex());
            require(sensors->has_batterystate() && sensors->batterystate().present(),
                    "Recovered battery missing after unary snapshot");
            require(sensors->batterystate().percentage() == 0.76f,
                    "Recovered battery percentage was not preserved");
            require(sensors->batterystate().power_supply_status() == Power::BatteryState::CHARGING,
                    "Recovered battery charging state was not preserved");
        }

        // Explicit pack removal from ROS must be distinct from 0% charge.
        // Legacy clients may send a present=false BatteryState field instead
        // of omitting it; the workstation must preserve that distinction.
        {
            Robot::Sensors removed;
            removed.mutable_robot_info()->set_robot_id("unit-robot");
            removed.mutable_batterystate()->set_present(false);
            removed.mutable_batterystate()->set_percentage(0.76f);
            Robot::Controls reply;
            grpc::ClientContext context;
            require(stub->DataExchange(&context, removed, &reply).ok(),
                    "Explicitly absent battery unary snapshot failed");
            std::lock_guard<std::mutex> lock(service.getMutex());
            require(sensors->has_batterystate(),
                    "Legacy explicitly absent battery field was dropped on the wire");
            require(!sensors->batterystate().present(),
                    "Explicitly absent battery was incorrectly marked present");
            require(sensors->robot_info().robot_id() == "unit-robot",
                    "Explicit battery removal damaged robot identity");
        }

        {
            grpc::ClientContext context;
            context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));
            auto stream = stub->MapStream(&context);
            map_service::GetMapResponse update;
            update.set_map_seq(42);
            require(stream && stream->Write(update), "Cannot send map update");
            map_service::GetMapRequest ack;
            require(stream->Read(&ack), "Map service did not acknowledge snapshot");
            // Keep the same RPC alive across a second map snapshot. The
            // server must acknowledge both frames and retain the newest map.
            update.set_map_seq(43);
            require(stream->Write(update), "Cannot send second map update");
            require(stream->Read(&ack), "Map service did not acknowledge second snapshot");
            stream->WritesDone();
            require(stream->Finish().ok(), "Map stream RPC failed");
            std::lock_guard<std::mutex> lock(service.getMapMutex());
            require(map->map_seq() == 43, "Latest map snapshot was not retained");
        }
        // Pose updates must acknowledge each frame on the SAME live RPC.
        // A new position must not reset the occupancy grid snapshot.
        {
            grpc::ClientContext context;
            context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));
            auto stream = stub->PoseStream(&context);
            map_service::PoseState update;
            update.mutable_pose()->set_position_x(1.25f);
            require(stream && stream->Write(update), "Cannot send first pose update");
            map_service::PoseRequest ack;
            require(stream->Read(&ack), "Pose service did not acknowledge first update");
            update.mutable_pose()->set_position_x(-2.5f);
            require(stream->Write(update), "Cannot send second pose update");
            require(stream->Read(&ack), "Pose service did not acknowledge second update");
            stream->WritesDone();
            require(stream->Finish().ok(), "Pose stream RPC failed");
            std::lock_guard<std::mutex> lock(service.getMapMutex());
            require(map->robotpose().position_x() == -2.5f,
                    "Latest pose was not retained");
            require(map->map_seq() == 43, "Pose update reset map snapshot");
        }

        // Zone removal is a real empty snapshot, not an idle/no-change event.
        // It must clear the previous zone list without erasing map or pose.
        {
            grpc::ClientContext context;
            context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));
            auto stream = stub->ZoneStream(&context);
            map_service::ZoneState update;
            update.mutable_zone_map()->add_zones()->set_id(7);
            require(stream && stream->Write(update), "Cannot send populated zone map");
            map_service::ZoneRequest ack;
            require(stream->Read(&ack), "Zone service did not acknowledge populated map");
            update.clear_zone_map();
            require(stream->Write(update), "Cannot send empty zone map");
            require(stream->Read(&ack), "Zone service did not acknowledge empty map");
            // Recovery on the same RPC must restore the new zone without
            // resurrecting the zone removed by the empty snapshot.
            update.mutable_zone_map()->add_zones()->set_id(19);
            require(stream->Write(update), "Cannot restore zone map on active RPC");
            require(stream->Read(&ack), "Zone service did not acknowledge restored map");
            stream->WritesDone();
            require(stream->Finish().ok(), "Zone stream RPC failed");
            std::lock_guard<std::mutex> lock(service.getMapMutex());
            require(map->zone_map().zones_size() == 1 &&
                    map->zone_map().zones(0).id() == 19,
                    "Restored zone snapshot retained stale zones or lost recovery");
            require(map->map_seq() == 43, "Zone update reset map snapshot");
            require(map->robotpose().position_x() == -2.5f,
                    "Zone update reset latest pose");
        }
        // A map-only snapshot must not erase pose and zones that arrived
        // on their independent streams. The legacy server assigned the whole
        // GetMapResponse and silently cleared both fields here.
        {
            grpc::ClientContext context;
            context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));
            auto stream = stub->MapStream(&context);
            map_service::GetMapResponse update;
            update.set_map_seq(44);
            require(stream && stream->Write(update), "Cannot send map-only refresh");
            map_service::GetMapRequest ack;
            require(stream->Read(&ack), "Map-only refresh was not acknowledged");
            stream->WritesDone();
            require(stream->Finish().ok(), "Map-only refresh RPC failed");
            std::lock_guard<std::mutex> lock(service.getMapMutex());
            require(map->map_seq() == 44, "Map-only refresh did not advance map sequence");
            require(map->robotpose().position_x() == -2.5f,
                    "Map-only refresh erased independently streamed pose");
            require(map->zone_map().zones_size() == 1 && map->zone_map().zones(0).id() == 19,
                    "Map-only refresh erased independently streamed zones");
        }
    } catch (const std::exception &e) {
        server->Shutdown();
        std::cerr << "gRPC transport smoke failed: " << e.what() << std::endl;
        return 1;
    }
    server->Shutdown();
    std::cout << "gRPC legacy/v2 stream, map and UI-mutex smoke: PASS" << std::endl;
    return 0;
}
