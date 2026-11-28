#include "getstatebyjoy.h"
#include <algorithm>

GetStateByJoystick::GetStateByJoystick(const std::shared_ptr<JoyState> state,
                                       std::uint64_t generation, QObject *parent)
    : QObject(parent), generation_(generation), joyState(state)
{
    int id = -1;
    {
        std::lock_guard<std::mutex> lock(*joyState->mutex);
        if (joyState->generation == generation_) id = joyState->joyId;
    }
    joyAdapter = JoystickFactory::createAdapter();
    if (id >= 0 && joyAdapter && joyAdapter->open(id)) {
        pollTimer = new QTimer(this);
        connect(pollTimer, &QTimer::timeout, this, &GetStateByJoystick::poll);
        pollTimer->start(LoopTime);
        poll();
    } else {
        invalidate();
    }
}

GetStateByJoystick::~GetStateByJoystick()
{
    stop();
}

void GetStateByJoystick::invalidate()
{
    std::lock_guard<std::mutex> lock(*joyState->mutex);
    if (joyState->generation != generation_) return;
    joyState->connected = false;
    joyState->sample_time = {};
    joyState->buttonVector.clear();
    joyState->joystickXaxis = joyState->joystickYaxis = joyState->joystickXrotation = 0;
    joyState->joystickYrotation = joyState->joystickZLTaxis = joyState->joystickZRTaxis = 0;
    joyState->joystickPOV0 = 0;
}

void GetStateByJoystick::stop()
{
    if (pollTimer) pollTimer->stop();
    invalidate();
    if (joyAdapter) joyAdapter->close();
}

void GetStateByJoystick::poll()
{
    // Read current device state, not cached events. A stationary stick is
    // still a fresh sample; an unplugged device cannot refresh the deadman.
    if (!joyAdapter || !joyAdapter->isConnected()) {
        invalidate();
        if (pollTimer) pollTimer->stop();
        if (joyAdapter) joyAdapter->close();
        return;
    }
    const auto now = std::chrono::steady_clock::now();
    const int n = std::clamp(joyAdapter->getJoystickNumButtons(), 0, int(MAX_JOYSTICK_BUTTONS));
    QVector<bool> buttons(n);
    for (int i = 0; i < n; ++i) buttons[i] = joyAdapter->button(i);
    const int x = joyAdapter->axis(0), y = -joyAdapter->axis(1);
    const int lt = joyAdapter->axis(2), rx = joyAdapter->axis(3);
    const int ry = -joyAdapter->axis(4), rt = joyAdapter->axis(5);
    if (!joyAdapter->isConnected()) { invalidate(); return; }
    std::lock_guard<std::mutex> lock(*joyState->mutex);
    if (joyState->generation != generation_) return;
    joyState->buttonVector = buttons;
    joyState->joystickXaxis = x;
    joyState->joystickYaxis = y;
    joyState->joystickZLTaxis = lt;
    joyState->joystickXrotation = rx;
    joyState->joystickYrotation = ry;
    joyState->joystickZRTaxis = rt;
    joyState->connected = true;
    joyState->sample_time = now;
}
