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

        {
            grpc::ClientContext context;
            context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));
            auto stream = stub->MapStream(&context);
            map_service::GetMapResponse update;
            update.set_map_seq(42);
            require(stream && stream->Write(update), "Cannot send map update");
            map_service::GetMapRequest ack;
            require(stream->Read(&ack), "Map service did not acknowledge snapshot");
            stream->WritesDone();
            require(stream->Finish().ok(), "Map stream RPC failed");
            std::lock_guard<std::mutex> lock(service.getMapMutex());
            require(map->map_seq() == 42, "Map snapshot was not retained");
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
