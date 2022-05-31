#include "joystickstateworker.h"

JoystickStateWorker::JoystickStateWorker(JoyState * const state):
    joyState(state)
{
}

JoystickStateWorker::~JoystickStateWorker()
{
    if (joyStateController != nullptr)
        delete joyStateController;
}

void JoystickStateWorker::process()
{
    joyStateController = new GetStateByJoystick(joyState);
}

void JoystickStateWorker::stop()
{
    if(joyStateController != nullptr)
            joyStateController->stop();
}
