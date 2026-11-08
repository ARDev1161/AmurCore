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
    auto map = std::make_shared<map_service::GetMapResponse>();
    grpcServer svc;
    svc.setProtosPointers(controls, observed, map);
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
        grpcClient agent(channel, agent_controls, agent_sensors, map, false);
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
        grpcClient fresh(fresh_channel, fresh_controls, fresh_sensors, map, false);
        require(fresh.DataExchange().ok(), "Reconnect first snapshot failed");
        check(svc, *observed, "robot-001", 1.0);
        {
            std::lock_guard<std::mutex> guard(*fresh.getMutex());
            require(fresh_controls->basecontrol().linearvelocity() == 4.5,
                    "Legacy controls lost on reconnect");
        }
        require(notifications.load() == 4 && lock_free.load(),
                "Missing callback or callback holds GUI mutex");

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
