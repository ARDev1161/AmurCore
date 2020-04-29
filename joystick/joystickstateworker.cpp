#include "joystickstateworker.h"

JoystickStateWorker::JoystickStateWorker(int id, AmurControls * const controls, JoyState * const state):
    joyId(id),
    joyState(state),
    amurControl(controls)
{

}

JoystickStateWorker::~JoystickStateWorker()
{
    if (joyStateController != nullptr)
        delete joyStateController;
}

void JoystickStateWorker::process()
{
    joyStateController = new GetStateByJoystick(joyId, amurControl, joyState);
}

void JoystickStateWorker::stop()
{
    if(joyStateController != nullptr)
            joyStateController->stop();
}

void JoystickStateWorker::changeJoystickId(int id)
{
    joyId = id;
}
