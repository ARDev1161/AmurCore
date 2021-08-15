#include "movements.h"

Movements::Movements(JoyState *joyState, AMUR::AmurControls *controls):
    joyState(joyState),
    controls(controls)
{

}

int Movements::joyTranslate()
{
    if(joyState->joyId < 0)
        return joyState->joyId;

    controls->mutable_handmotors()->set_rightrelay( joyState->buttonVector[5] );
    controls->mutable_handmotors()->set_leftrelay( joyState->buttonVector[4] );

    std::cout << controls->DebugString() << std::endl;

    return 0;
}
