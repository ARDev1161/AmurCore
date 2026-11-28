#include "movements.h"
#include <algorithm>
#include <cmath>

namespace {
double normalized(int raw)
{
    constexpr double deadzone = 0.06;
    const double v = std::clamp(raw / 32767.0, -1.0, 1.0);
    if (std::abs(v) <= deadzone) return 0.0;
    return std::copysign((std::abs(v) - deadzone) / (1.0 - deadzone), v);
}
}

ManualControl::ManualControl(std::shared_ptr<JoyState> joyState,
                             std::shared_ptr<Controls> controls,
                             std::shared_ptr<Sensors> sensors,
                             std::mutex &grpcMutex, TeleopIntent &intent, int deadmanButton)
    : controls(controls), sensors(sensors), grpcMutex_(grpcMutex), intent_(intent),
      joyState(joyState), deadmanButton_(deadmanButton)
{}

int ManualControl::update()
{
    JoyState input;
    {
        std::lock_guard<std::mutex> lock(*joyState->mutex);
        input = *joyState;
    }
    std::lock_guard<std::mutex> lock(grpcMutex_);
    controls->clear_basecontrol();
    controls->clear_velocity_command();
    controls->clear_request_velocity_lease();
    intent_ = {};
    const auto now = TeleopIntent::Clock::now();
    if (!input.connected || input.sample_time > now ||
        now - input.sample_time >= TeleopIntent::max_age) {
        releaseSeen_ = false;
        lastJoyState = input;
        return -1;
    }
    if (!lastJoyState.connected || input.generation != lastJoyState.generation)
        releaseSeen_ = false;
    const auto robot = sensors->has_robot_info() ? sensors->robot_info().robot_id() : std::string{};
    if (robot != robotId_) { releaseSeen_ = false; robotId_ = robot; }
    baseControlProcess(input);
    // Existing unrelated peripheral controls remain unchanged.
    checkChangeRelayButton(input, 5);
    checkChangeLightButton(input, 0);
    moveCameraProcess(input.joystickZLTaxis / 129, input.joystickXrotation / 129);
    lastJoyState = input;
    return 0;
}

void ManualControl::baseControlProcess(const JoyState &input)
{
    using namespace rc::api::v2;
    if (deadmanButton_ < 0 || deadmanButton_ >= input.buttonVector.size()) {
        releaseSeen_ = false;
        return;
    }
    const bool pressed = input.buttonVector[deadmanButton_];
    intent_.sample_time = input.sample_time;
    intent_.deadman_released = !pressed;
    if (!pressed) { releaseSeen_ = true; return; }
    if (!releaseSeen_ || !sensors->has_robot_info() || !sensors->robot_info().has_base()) return;
    const auto &info = sensors->robot_info();
    const auto &base = info.base();
    if (info.api_major() != 2 || base.command_frame_id().empty() || base.axis_limits().empty()) return;
    controls->set_request_velocity_lease(true);
    const auto &lease = sensors->velocity_lease();
    if (!sensors->has_velocity_lease() || !lease.granted() ||
        lease.robot_id() != info.robot_id() || lease.session_id().empty() || lease.lease_id().empty()) return;
    auto *cmd = controls->mutable_velocity_command();
    cmd->set_robot_id(info.robot_id());
    cmd->set_session_id(lease.session_id());
    cmd->set_lease_id(lease.lease_id());
    cmd->set_deadman(true);
    auto *v = cmd->mutable_velocity();
    v->set_frame_id(base.command_frame_id());
    v->mutable_linear();
    v->mutable_angular();
    bool omni = false;
    for (const auto &limit : base.axis_limits()) if (limit.axis() == LINEAR_Y) omni = true;
    for (const auto &limit : base.axis_limits()) {
        if (!std::isfinite(limit.min_velocity()) || !std::isfinite(limit.max_velocity()) ||
            limit.min_velocity() > 0.0 || limit.max_velocity() < 0.0) {
            controls->clear_velocity_command();
            controls->clear_request_velocity_lease();
            releaseSeen_ = false;
            return;
        }
        double n = 0.0;
        switch (limit.axis()) {
        case LINEAR_X: n = normalized(input.joystickYaxis); break;
        case LINEAR_Y: n = -normalized(input.joystickXaxis); break;
        case ANGULAR_Z: n = -normalized(omni ? input.joystickXrotation : input.joystickXaxis); break;
        default: break; // Unmapped axes remain zero; no invented kinematics.
        }
        const double si = n >= 0.0 ? n * limit.max_velocity() : -n * limit.min_velocity();
        switch (limit.axis()) {
        case LINEAR_X: v->mutable_linear()->set_x(si); break;
        case LINEAR_Y: v->mutable_linear()->set_y(si); break;
        case ANGULAR_Z: v->mutable_angular()->set_z(si); break;
        default: break;
        }
    }
    // Sequence and remaining TTL are assigned on each authenticated stream
    // response, under this same mutex, using the original sample timestamp.
}

void ManualControl::checkChangeRelayButton(const JoyState &input, int buttonNumber)
{
    if (buttonNumber < 0 || static_cast<size_t>(buttonNumber) >= input.buttonVector.size() ||
        static_cast<size_t>(buttonNumber) >= lastJoyState.buttonVector.size())
        return;

    bool state = input.buttonVector[buttonNumber];

    if(state != lastJoyState.buttonVector[buttonNumber])
    {
        controls->mutable_handmotors()->set_rightrelay( state );
        controls->mutable_handmotors()->set_leftrelay( state );
    }
}

void ManualControl::checkChangeLightButton(const JoyState &input, int buttonNumber)
{
    if (buttonNumber < 0 || static_cast<size_t>(buttonNumber) >= input.buttonVector.size() ||
        static_cast<size_t>(buttonNumber) >= lastJoyState.buttonVector.size())
        return;

    bool state = input.buttonVector[buttonNumber];

    if(state != lastJoyState.buttonVector[buttonNumber])
    {
        controls->mutable_light()->set_ledleftpower( 255 * (int)state );
        controls->mutable_light()->set_ledrightpower( 255 * (int)state );
    }
}

int ManualControl::moveCameraProcess(int xAxis, int yAxis)
{
    controls->mutable_cameraservos()->set_xangle( xAxis );
    controls->mutable_cameraservos()->set_yangle( yAxis );

    return 0;
}
