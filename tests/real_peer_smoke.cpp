// P1: exercise real rc_agent grpcClient against real AmurCore grpcServer.
// Both repositories have network/client.h and network/server.h. Include the
// intended implementation explicitly; avoid compiling against the wrong class.
#ifdef RC_AGENT_ROOT
#include "peer/network/server.h"
#include "network/client.h"
#else
#include "network/server.h"
#include "peer/network/client.h"
#endif
#include <atomic>
#include <chrono>
#include <iostream>
#include <future>
#include <thread>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>

namespace {
void require(bool ok, const char *why) { if (!ok) throw std::runtime_error(why); }
void advertise(Robot::Sensors &msg) {
    auto *info = msg.mutable_robot_info();
    info->set_robot_id("robot-001");
    info->set_api_major(2);
    auto *cap = info->add_capabilities();
    cap->set_id("manual.velocity");
    cap->add_velocity_axes("linear.x");
    cap->add_velocity_axes("angular.z");
}
void check(grpcServer &svc, const Robot::Sensors &observed,
           const std::string &robot_id, double battery) {
    std::lock_guard<std::mutex> lock(svc.getMutex());
    require(observed.has_robot_info() == !robot_id.empty(), "Descriptor presence mismatch");
    require(observed.robot_info().robot_id() == robot_id, "Stale robot ID");
    require(observed.batterystate().percentage() == battery, "Stale sensor snapshot");
    if (!robot_id.empty()) {
        const auto &info = observed.robot_info();
        require(info.api_major() == 2 && info.capabilities_size() == 1, "Lost v2 descriptor");
        const auto &cap = info.capabilities(0);
        require(cap.id() == "manual.velocity" && cap.velocity_axes_size() == 2 &&
                cap.velocity_axes(0) == "linear.x" && cap.velocity_axes(1) == "angular.z",
                "Lost supported axes");
    }
}
}
int main() {
    auto controls = std::make_shared<Robot::Controls>();
    auto observed = std::make_shared<Robot::Sensors>();
    // Distinct protobuf storage is essential: map transport tests must not
    // accidentally pass merely because both peers share an in-process pointer.
    auto operator_map = std::make_shared<map_service::GetMapResponse>();
    auto agent_map = std::make_shared<map_service::GetMapResponse>();
    grpcServer svc;
    svc.setProtosPointers(controls, observed, operator_map);
    std::atomic<unsigned> notifications{0};
    std::atomic<bool> lock_free{true};
    svc.setSensorsUpdatedCallback([&]() {
        // An unrelated RPC may briefly own the mutex when this callback
        // runs. A single try_lock would report a false positive. Probe from
        // another thread with a bounded deadline: it can acquire the mutex
        // after ordinary contention, but not while this callback's own RPC
        // is still holding the mutex.
        auto probe = std::async(std::launch::async, [&]() {
            const auto deadline = std::chrono::steady_clock::now() +
                                  std::chrono::milliseconds(250);
            do {
                if (svc.getMutex().try_lock()) {
                    svc.getMutex().unlock();
                    return true;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            } while (std::chrono::steady_clock::now() < deadline);
            return false;
        });
        if (!probe.get()) lock_free = false;
        ++notifications;
    });
    {
        std::lock_guard<std::mutex> guard(svc.getMutex());
        controls->mutable_basecontrol()->set_linearvelocity(4.5);
    }
    int port = 0;
    grpc::ServerBuilder builder;
    builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &port);
    builder.RegisterService(&svc);
    auto server = builder.BuildAndStart();
    require(server != nullptr && port > 0, "Cannot start AmurCore server");
    try {
        const auto address = "127.0.0.1:" + std::to_string(port);
        auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
        require(channel->WaitForConnected(std::chrono::system_clock::now() +
                                          std::chrono::seconds(3)), "Cannot connect");
        auto agent_sensors = std::make_shared<Robot::Sensors>();
        auto agent_controls = std::make_shared<Robot::Controls>();
        grpcClient agent(channel, agent_controls, agent_sensors, agent_map, false);
        agent_sensors->mutable_batterystate()->set_percentage(0.25);
        agent_sensors->mutable_video_stream()->set_pipeline(
            "videotestsrc ! fakesink");
        require(agent.DataExchange().ok(), "Legacy DataExchange failed");
        {
            std::lock_guard<std::mutex> guard(svc.getMutex());
            require(observed->video_stream().pipeline() ==
                        "videotestsrc ! fakesink",
                    "Legacy video pipeline was not transmitted to operator");
        }
        check(svc, *observed, "", 0.25);
        {
            std::lock_guard<std::mutex> guard(*agent.getMutex());
            require(agent_controls->basecontrol().linearvelocity() == 4.5,
                    "Legacy controls reply changed");
        }
        advertise(*agent_sensors);
        agent_sensors->mutable_batterystate()->set_percentage(0.5);
        agent_sensors->mutable_video_stream()->set_pipeline(
            "videotestsrc pattern=ball ! fakesink");
        require(agent.DataExchange().ok(), "V2 DataExchange failed");
        {
            std::lock_guard<std::mutex> guard(svc.getMutex());
            require(observed->video_stream().pipeline() ==
                        "videotestsrc pattern=ball ! fakesink",
                    "Updated video pipeline was not transmitted with v2 RobotInfo");
            require(observed->robot_info().robot_id() == "robot-001",
                    "Video update erased advertised robot identity");
        }
        check(svc, *observed, "robot-001", 0.5);
        agent_sensors->clear_robot_info();
        agent_sensors->mutable_batterystate()->set_percentage(0.75);
        require(agent.DataExchange().ok(), "Legacy fallback failed");
        check(svc, *observed, "", 0.75);
        auto fresh_sensors = std::make_shared<Robot::Sensors>();
        fresh_sensors->mutable_batterystate()->set_percentage(1.0);
        advertise(*fresh_sensors);
        auto fresh_controls = std::make_shared<Robot::Controls>();
        auto fresh_channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
        grpcClient fresh(fresh_channel, fresh_controls, fresh_sensors, agent_map, false);
        require(fresh.DataExchange().ok(), "Reconnect first snapshot failed");
        check(svc, *observed, "robot-001", 1.0);
        {
            std::lock_guard<std::mutex> guard(svc.getMutex());
            require(observed->video_stream().pipeline().empty(),
                    "New client snapshot retained previous robot's video pipeline");
        }
        {
            std::lock_guard<std::mutex> guard(*fresh.getMutex());
            require(fresh_controls->basecontrol().linearvelocity() == 4.5,
                    "Legacy controls lost on reconnect");
        }
        require(notifications.load() == 4 && lock_free.load(),
                "Missing callback or callback holds GUI mutex");

        // BatteryState is optional: a v2 peer may have no battery sensor.
        // Clearing it on the next unary snapshot must not retain stale data
        // or erase the independent RobotInfo descriptor.
        {
            std::lock_guard<std::mutex> guard(*fresh.getMutex());
            fresh_sensors->clear_batterystate();
        }
        require(fresh.DataExchange().ok(), "Battery-less v2 unary exchange failed");
        {
            std::lock_guard<std::mutex> guard(svc.getMutex());
            require(!observed->has_batterystate(),
                    "Battery-less snapshot retained stale BatteryState");
            require(observed->robot_info().robot_id() == "robot-001" &&
                        observed->robot_info().api_major() == 2,
                    "Battery-less snapshot lost v2 RobotInfo");
        }
        require(notifications.load() == 5 && lock_free.load(),
                "Battery-less exchange missed callback or held UI mutex");

        // A sensor may recover after a battery-less snapshot. Ensure the
        // restored optional field carries fresh values and keeps RobotInfo.
        {
            std::lock_guard<std::mutex> guard(*fresh.getMutex());
            fresh_sensors->mutable_batterystate()->set_percentage(0.42f);
            fresh_sensors->mutable_batterystate()->set_present(true);
        }
        require(fresh.DataExchange().ok(), "Restored battery unary exchange failed");
        {
            std::lock_guard<std::mutex> guard(svc.getMutex());
            require(observed->has_batterystate() &&
                        observed->batterystate().percentage() > 0.419f &&
                        observed->batterystate().percentage() < 0.421f,
                    "Restored battery snapshot lost updated BatteryState");
            require(observed->robot_info().robot_id() == "robot-001" &&
                        observed->robot_info().api_major() == 2,
                    "Restored battery snapshot lost v2 RobotInfo");
        }
        require(notifications.load() == 6 && lock_free.load(),
                "Restored battery exchange missed callback or held UI mutex");

        // The real rc_agent stream must terminate when stopStream() is
        // requested, rather than leave a blocking network Read alive.
        {
            std::lock_guard<std::mutex> guard(*agent.getMutex());
            agent_sensors->mutable_video_stream()->set_pipeline(
                "videotestsrc pattern=snow ! fakesink");
        }
        auto live_stream = std::async(std::launch::async, [&agent]() {
            return agent.DataStreamExchange();
        });
        for (int i = 0; i < 300 && notifications.load() < 7; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        require(notifications.load() >= 7,
                "Real peer did not transmit the streaming snapshot");
        {
            std::lock_guard<std::mutex> guard(svc.getMutex());
            require(observed->video_stream().pipeline() ==
                        "videotestsrc pattern=snow ! fakesink",
                    "Legacy stream did not deliver updated video pipeline");
        }
        agent.stopStream();
        require(live_stream.wait_for(std::chrono::seconds(3)) ==
                    std::future_status::ready,
                "stopStream did not release real peer stream");
        const auto stopped_status = live_stream.get();
        require(stopped_status.error_code() == grpc::StatusCode::CANCELLED,
                "Stopped real peer stream did not report cancellation");
        require(lock_free.load(), "Stopped peer callback held UI mutex");

        // Real legacy map stream: verify an initial coherent snapshot and
        // a changed sequence reach the workstation across gRPC (not shared
        // protobuf pointers). No full-map comparison is performed.
        auto map_channel = grpc::CreateChannel(
            address, grpc::InsecureChannelCredentials());
        grpcClient map_client(map_channel, agent_controls, agent_sensors,
                              agent_map, false);
        {
            std::lock_guard<std::mutex> guard(*map_client.getMapMutex());
            agent_map->set_map_seq(71);
            agent_map->mutable_robotpose()->set_position_x(1.25);
        }
        auto map_rpc = std::async(std::launch::async, [&map_client]() {
            return map_client.MapStream();
        });
        auto waitMapSeq = [&](std::uint64_t seq, double position) {
            for (int i = 0; i < 500; ++i) {
                {
                    std::lock_guard<std::mutex> guard(svc.getMapMutex());
                    if (operator_map->map_seq() == seq &&
                        operator_map->robotpose().position_x() == position) {
                        return true;
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            return false;
        };
        const bool initial_map_received = waitMapSeq(71, 1.25);
        {
            std::lock_guard<std::mutex> guard(*map_client.getMapMutex());
            agent_map->set_map_seq(72);
            agent_map->mutable_robotpose()->set_position_x(2.50);
        }
        const bool updated_map_received = waitMapSeq(72, 2.50);
        bool map_mutex_available = map_client.getMapMutex()->try_lock();
        if (map_mutex_available) {
            map_client.getMapMutex()->unlock();
        }
        map_client.stopStream();
        const bool map_stopped =
            map_rpc.wait_for(std::chrono::seconds(3)) ==
            std::future_status::ready;
        require(initial_map_received, "Real map stream did not send the initial map");
        require(updated_map_received, "Real map stream did not send updated map sequence");
        require(map_mutex_available, "Real map stream held mutex while idle");
        require(map_stopped, "stopStream failed to cancel real map RPC");
        require(map_rpc.get().error_code() == grpc::StatusCode::CANCELLED,
                "Cancelled map RPC returned the wrong status");

        // Pose and zone use their own LEGACY streams. MapStream is already
        // stopped, so these values cannot arrive through a full-map snapshot.
        // Keep sender/receiver map pointers independent to exercise real gRPC.
        auto pose_channel = grpc::CreateChannel(
            address, grpc::InsecureChannelCredentials());
        grpcClient pose_client(pose_channel, agent_controls, agent_sensors,
                               agent_map, false);
        {
            std::lock_guard<std::mutex> guard(*pose_client.getMapMutex());
            agent_map->mutable_robotpose()->set_position_x(3.75f);
        }
        auto pose_rpc = std::async(std::launch::async, [&pose_client]() {
            return pose_client.PoseStream();
        });
        auto waitPose = [&](float x) {
            for (int i = 0; i < 500; ++i) {
                {
                    std::lock_guard<std::mutex> guard(svc.getMapMutex());
                    if (operator_map->robotpose().position_x() == x)
                        return true;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            return false;
        };
        const bool pose_initial = waitPose(3.75f);
        {
            std::lock_guard<std::mutex> guard(*pose_client.getMapMutex());
            agent_map->mutable_robotpose()->set_position_x(4.25f);
        }
        const bool pose_updated = waitPose(4.25f);
        pose_client.stopStream();
        const bool pose_finished =
            pose_rpc.wait_for(std::chrono::seconds(3)) == std::future_status::ready;
        require(pose_initial, "Initial pose did not arrive via PoseStream");
        require(pose_updated, "Updated pose did not arrive via PoseStream");
        require(pose_finished, "PoseStream did not stop on cancellation");
        require(pose_rpc.get().error_code() == grpc::StatusCode::CANCELLED,
                "Canceled PoseStream status mismatch");
        {
            std::lock_guard<std::mutex> guard(svc.getMapMutex());
            require(operator_map->map_seq() == 72,
                    "PoseStream unexpectedly replaced OccupancyGrid/map_seq");
        }

        auto zone_channel = grpc::CreateChannel(
            address, grpc::InsecureChannelCredentials());
        grpcClient zone_client(zone_channel, agent_controls, agent_sensors,
                               agent_map, false);
        {
            std::lock_guard<std::mutex> guard(*zone_client.getMapMutex());
            agent_map->set_zone_seq(801);
            auto *zone = agent_map->mutable_zone_map()->add_zones();
            zone->set_id(1001);
            zone->set_name("alpha");
        }
        auto zone_rpc = std::async(std::launch::async, [&zone_client]() {
            return zone_client.ZoneStream();
        });
        auto waitZone = [&](const std::string &name) {
            for (int i = 0; i < 500; ++i) {
                {
                    std::lock_guard<std::mutex> guard(svc.getMapMutex());
                    const auto &zones = operator_map->zone_map();
                    if (zones.zones_size() == 1 && zones.zones(0).id() == 1001 &&
                        zones.zones(0).name() == name)
                        return true;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            return false;
        };
        const bool zone_initial = waitZone("alpha");
        {
            std::lock_guard<std::mutex> guard(*zone_client.getMapMutex());
            agent_map->set_zone_seq(802);
            agent_map->mutable_zone_map()->mutable_zones(0)->set_name("beta");
        }
        const bool zone_updated = waitZone("beta");
        zone_client.stopStream();
        const bool zone_finished =
            zone_rpc.wait_for(std::chrono::seconds(3)) == std::future_status::ready;
        require(zone_initial, "Initial zone did not arrive via ZoneStream");
        require(zone_updated, "Updated zone did not arrive via ZoneStream");
        require(zone_finished, "ZoneStream did not stop on cancellation");
        require(zone_rpc.get().error_code() == grpc::StatusCode::CANCELLED,
                "Canceled ZoneStream status mismatch");
        {
            std::lock_guard<std::mutex> guard(svc.getMapMutex());
            require(operator_map->map_seq() == 72 &&
                        operator_map->robotpose().position_x() == 4.25f,
                    "ZoneStream unexpectedly modified map or pose");
        }

        // A stopped rc_agent client must also reject the legacy unary path,
        // without sending stale telemetry or changing the last controls.
        auto stopped_channel = grpc::CreateChannel(
            address, grpc::InsecureChannelCredentials());
        grpcClient stopped_client(stopped_channel, agent_controls, agent_sensors,
                                  agent_map, true);
        stopped_client.stopStream();
        const auto stopped_start = std::chrono::steady_clock::now();
        require(stopped_client.DataExchange().error_code() ==
                    grpc::StatusCode::CANCELLED,
                "Stopped real peer restarted unary DataExchange");
        require(std::chrono::steady_clock::now() - stopped_start <
                    std::chrono::seconds(1),
                "Stopped unary peer did not return promptly");
        {
            std::lock_guard<std::mutex> guard(*stopped_client.getMutex());
            require(agent_controls->basecontrol().linearvelocity() == 4.5,
                    "Stopped unary peer modified previously valid controls");
        }

        // P1 reconnect failure: a cleanly shut down operator must not make
        // one-shot rc_agent DataExchange block forever or inject empty controls.
        server->Shutdown();
        const auto disconnect_start = std::chrono::steady_clock::now();
        const grpc::Status failed_rpc = fresh.DataExchange();
        require(!failed_rpc.ok(), "Disconnected operator falsely accepted telemetry");
        require(std::chrono::steady_clock::now() - disconnect_start <
                    std::chrono::seconds(4),
                "One-shot real peer did not return after operator disconnect");
        {
            std::lock_guard<std::mutex> guard(*fresh.getMutex());
            require(fresh_controls->basecontrol().linearvelocity() == 4.5,
                    "Disconnected peer overwrote the previous controls");
        }
    } catch (const std::exception &e) {
        server->Shutdown();
        std::cerr << "Real peer test failed: " << e.what() << '\n';
        return 1;
    }
    server->Shutdown();
    std::cout << "Real AmurCore/rc_agent legacy-v2-reconnect PASS\n";
    return 0;
}
