#include "joystickstateworker.h"

JoystickStateWorker::JoystickStateWorker(int id, JoyState * const state):
    joyId(id),
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
    joyStateController = new GetStateByJoystick(joyId, joyState);
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
