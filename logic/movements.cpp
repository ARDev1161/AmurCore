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

    // The old Controls.BaseControl velocity is not a v2 command: it has no
    // session, lease, TTL or deadman. Keep the generic stream at zero until
    // the dedicated authorized v2 velocity sender is connected.
    controls->mutable_basecontrol()->set_linearvelocity(0.0);
    controls->mutable_basecontrol()->set_angularvelocity(0.0);
    // v2 operator controls must not emit raw actuator power commands.
    moveCameraProcess(moveSettings.joyBindings.cameraXAxis(*joyState),
                     moveSettings.joyBindings.cameraYAxis(*joyState));

    lastJoyState = *joyState;
    return 0;
}

void ManualControl::setBaseControlLevel(Base::BaseControl::ControlLevel newBaseControlLevel)
{
    baseControlLevel = newBaseControlLevel;
}

void ManualControl::checkChangeRelayButton(int buttonNumber)
{
    bool state = joyState->buttonVector[buttonNumber];

    if(state != lastJoyState.buttonVector[buttonNumber])
    {
        controls->mutable_handmotors()->set_rightrelay( state );
        controls->mutable_handmotors()->set_leftrelay( state );
    }
}

void ManualControl::checkChangeLightButton(int buttonNumber)
{
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
