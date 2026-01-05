#ifndef JOYSTICK_FACTORY_H
#define JOYSTICK_FACTORY_H

#include <memory>
#include "v_joystick_adapter.h"

class JoystickFactory
{
public:
    static std::unique_ptr<VJoystickAdapter> createAdapter()
    {
        return std::make_unique<VJoystickAdapter>();
    }
};

#endif // JOYSTICK_FACTORY_H
