#include "joystickidholder.h"

JoystickIdHolder::JoystickIdHolder()
{
    joyId = -1;
}

int JoystickIdHolder::getJoyId() const
{
    return joyId;
}

void JoystickIdHolder::setJoyId(int value)
{
    joyId = value;
}
