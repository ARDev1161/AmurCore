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
        if (svc.getMutex().try_lock()) svc.getMutex().unlock();
        else lock_free = false;
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
        require(agent.DataExchange().ok(), "Legacy DataExchange failed");
        check(svc, *observed, "", 0.25);
        {
            std::lock_guard<std::mutex> guard(*agent.getMutex());
            require(agent_controls->basecontrol().linearvelocity() == 4.5,
                    "Legacy controls reply changed");
        }
        advertise(*agent_sensors);
        agent_sensors->mutable_batterystate()->set_percentage(0.5);
        require(agent.DataExchange().ok(), "V2 DataExchange failed");
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
            std::lock_guard<std::mutex> guard(*fresh.getMutex());
            require(fresh_controls->basecontrol().linearvelocity() == 4.5,
                    "Legacy controls lost on reconnect");
        }
        require(notifications.load() == 4 && lock_free.load(),
                "Missing callback or callback holds GUI mutex");

        // The real rc_agent stream must terminate when stopStream() is
        // requested, rather than leave a blocking network Read alive.
        auto live_stream = std::async(std::launch::async, [&agent]() {
            return agent.DataStreamExchange();
        });
        for (int i = 0; i < 300 && notifications.load() < 5; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        require(notifications.load() >= 5,
                "Real peer did not transmit the streaming snapshot");
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
