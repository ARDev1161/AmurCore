#include "joystickstateworker.h"

JoystickStateWorker::JoystickStateWorker(const std::shared_ptr<JoyState> state):
    joyState(state)
{
    std::lock_guard<std::mutex> lock(*joyState->mutex);
    generation_ = joyState->generation;
}

JoystickStateWorker::~JoystickStateWorker()
{
    if (joyStateController != nullptr)
        delete joyStateController;
}

void JoystickStateWorker::process()
{
    joyStateController = new GetStateByJoystick(joyState, generation_, this);
}

void JoystickStateWorker::stop()
{
    if(joyStateController != nullptr)
            joyStateController->stop();
    emit finished();
}
