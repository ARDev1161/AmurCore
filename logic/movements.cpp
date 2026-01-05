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

    checkChangeRelayButton( moveSettings.joyBindings.relayButton );
    checkChangeLightButton( moveSettings.joyBindings.lightButton );

    const int wheelX = moveSettings.joyBindings.wheelXAxis(*joyState);
    const int wheelY = moveSettings.joyBindings.wheelYAxis(*joyState);

    baseControlProcess(wheelX, wheelY);
    wheelRawProcess(wheelX, wheelY);
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

int ManualControl::baseControlProcess(int xAxis, int yAxis)
{
    if(!baseControlStrategy)
        return -1;

    const auto [linear, angular] = baseControlStrategy->velocities(*joyState, moveSettings.joyBindings);

    controls->mutable_basecontrol()->set_linearvelocity(linear);
    controls->mutable_basecontrol()->set_angularvelocity(angular);

    controls->mutable_basecontrol()->set_controllevel(baseControlLevel);

    return 0;
}

int ManualControl::wheelRawProcess(int xAxis, int yAxis)
{
//    controls->mutable_wheelmotors()->set_leftpower( (xAxis+yAxis)/2 );
//    controls->mutable_wheelmotors()->set_rightpower( (-xAxis+yAxis)/2 );

//    controls->mutable_handmotors()->set_leftrelay(true);
//    controls->mutable_handmotors()->set_rightrelay(true);
    controls->mutable_handmotors()->set_leftpower( (xAxis) );
    controls->mutable_handmotors()->set_rightpower( (yAxis) );

    std::cout << "\nX = " << xAxis << "\tY = " << yAxis << std::endl;
//    std::cout << "l = " << controls->wheelmotors().leftpower() << "\tr = " << controls->wheelmotors().rightpower() << std::endl;
    std::cout << "l = " << controls->mutable_handmotors()->leftpower() << "\tr = " << controls->mutable_handmotors()->rightpower() << std::endl;

//    controls->mutable_wheelmotors()->set_lefttime(1000);
//    controls->mutable_wheelmotors()->set_righttime(1000);

    controls->mutable_handmotors()->set_lefttime(1000);
    controls->mutable_handmotors()->set_righttime(1000);

    return 0;
}

int ManualControl::moveCameraProcess(int xAxis, int yAxis)
{
    controls->mutable_cameraservos()->set_xangle( xAxis );
    controls->mutable_cameraservos()->set_yangle( yAxis );

    return 0;
}
