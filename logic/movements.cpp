#include "movements.h"

Movements::Movements(JoyState *joyState, AMUR::AmurControls *controls):
    controls(controls),
    joyState(joyState)
{

}

int Movements::update()
{
    if(joyState->joyId < 0)
         return joyState->joyId;

    if(lastJoyState.joyId < 0)
        lastJoyState = *joyState;

    checkChangeRelayButton( moveSettings.joyBindings.relayButton );
    checkChangeLightButton( moveSettings.joyBindings.lightButton );

    wheelProcess( (WHEEL_X_AXIS / DIVIDER) , (WHEEL_Y_AXIS / DIVIDER) );
    moveCameraProcess( (CAM_X_AXIS / DIVIDER) , (CAM_Y_AXIS / DIVIDER) );

    lastJoyState = *joyState;
    return 0;
}

void Movements::checkChangeRelayButton(int buttonNumber)
{
    bool state = joyState->buttonVector[buttonNumber];

    if(state != lastJoyState.buttonVector[buttonNumber])
    {
        controls->mutable_handmotors()->set_rightrelay( state );
        controls->mutable_handmotors()->set_leftrelay( state );
    }
}

void Movements::checkChangeLightButton(int buttonNumber)
{
    bool state = joyState->buttonVector[buttonNumber];

    if(state != lastJoyState.buttonVector[buttonNumber])
    {
        controls->mutable_light()->set_ledleftpower( 255 * (int)state );
        controls->mutable_light()->set_ledrightpower( 255 * (int)state );
    }
}

int Movements::wheelProcess(int xAxis, int yAxis)
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

int Movements::moveCameraProcess(int xAxis, int yAxis)
{
    controls->mutable_cameraservos()->set_xangle( xAxis );
    controls->mutable_cameraservos()->set_yangle( yAxis );

    return 0;
}
