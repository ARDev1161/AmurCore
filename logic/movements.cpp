#include "movements.h"
#include <cmath>

std::pair<double, double> DeadzoneBaseControlStrategy::velocities(const JoyState &state,
                                                                 const MoveSettings::JoyBindings &bindings) const
{
    const double rawAngular = bindings.wheelXAxis(state);
    const double rawLinear = bindings.wheelYAxis(state);

    const double linear = (std::abs(rawLinear) > driftZone_) ? rawLinear : 0.0;
    const double angular = (std::abs(rawAngular) > driftZone_) ? rawAngular : 0.0;

    return {linear, angular};
}

ManualControl::ManualControl(std::shared_ptr<JoyState> joyState,
                     std::shared_ptr<Controls> controls,
                     std::mutex &grpcMutex,
                     Base::BaseControl::ControlLevel baseControlLevel):
    controls(controls),
    joyState(joyState),
    grpcMutex_(grpcMutex),
    baseControlLevel(baseControlLevel)
{
    baseControlStrategy = std::make_unique<DeadzoneBaseControlStrategy>(joyDriftZone);

}

int ManualControl::update()
{
    if(joyState->joyId < 0)
         return joyState->joyId;

    if(lastJoyState.joyId < 0)
        lastJoyState = *joyState;

    std::lock_guard<std::mutex> lock(grpcMutex_);

    checkChangeRelayButton( moveSettings.joyBindings.relayButton );
    checkChangeLightButton( moveSettings.joyBindings.lightButton );

    const int wheelX = moveSettings.joyBindings.wheelXAxis(*joyState);
    const int wheelY = moveSettings.joyBindings.wheelYAxis(*joyState);
    baseControlProcess(wheelX, wheelY);
    // v2 operator controls must not emit raw actuator power commands.
    moveCameraProcess(moveSettings.joyBindings.cameraXAxis(*joyState),
                     moveSettings.joyBindings.cameraYAxis(*joyState));

    lastJoyState = *joyState;
    return 0;
}

int ManualControl::baseControlProcess(int, int)
{
    if (!baseControlStrategy)
        return -1;

    const auto [linear, angular] = baseControlStrategy->velocities(*joyState, moveSettings.joyBindings);
    controls->mutable_basecontrol()->set_linearvelocity(linear);
    controls->mutable_basecontrol()->set_angularvelocity(angular);
    controls->mutable_basecontrol()->set_controllevel(baseControlLevel);
    return 0;
}

void ManualControl::setBaseControlLevel(Base::BaseControl::ControlLevel newBaseControlLevel)
{
    baseControlLevel = newBaseControlLevel;
}

void ManualControl::checkChangeRelayButton(int buttonNumber)
{
    if (buttonNumber < 0 || static_cast<size_t>(buttonNumber) >= joyState->buttonVector.size() ||
        static_cast<size_t>(buttonNumber) >= lastJoyState.buttonVector.size())
        return;

    bool state = joyState->buttonVector[buttonNumber];

    if(state != lastJoyState.buttonVector[buttonNumber])
    {
        controls->mutable_handmotors()->set_rightrelay( state );
        controls->mutable_handmotors()->set_leftrelay( state );
    }
}

void ManualControl::checkChangeLightButton(int buttonNumber)
{
    if (buttonNumber < 0 || static_cast<size_t>(buttonNumber) >= joyState->buttonVector.size() ||
        static_cast<size_t>(buttonNumber) >= lastJoyState.buttonVector.size())
        return;

    bool state = joyState->buttonVector[buttonNumber];

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
