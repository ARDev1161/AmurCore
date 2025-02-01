#ifndef MOVEMENTS_H
#define MOVEMENTS_H

#include "../joystick/joystick.h"

using namespace Robot;

struct MoveSettings
{
    struct JoyBindings
    {
        int relayButton = 5;
        int lightButton = 0;

        #define WHEEL_X_AXIS joyState->joystickXaxis
        #define WHEEL_Y_AXIS joyState->joystickYaxis

        #define CAM_X_AXIS joyState->joystickZLTaxis
        #define CAM_Y_AXIS joyState->joystickXrotation

        #define HAND_AXIS joyState->joystickXrotation

        #define DIVIDER 129
    }
    joyBindings;
};

class Movements
{
    std::shared_ptr<Controls> controls;
    MoveSettings moveSettings;
    std::shared_ptr<JoyState> joyState;
    JoyState lastJoyState;

    void checkChangeRelayButton(int buttonNumber);
    void checkChangeLightButton(int buttonNumber);

    int wheelProcess(int xAxis, int yAxis);
    int moveCameraProcess(int xAxis, int yAxis);
public:
    Movements(std::shared_ptr<JoyState> joyState, std::shared_ptr<Controls> controls);
    int update();
};

#endif // MOVEMENTS_H
