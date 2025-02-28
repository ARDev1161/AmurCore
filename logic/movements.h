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

class ManualControl
{
    std::shared_ptr<Controls> controls;
    std::mutex &grpcMutex_;

    MoveSettings moveSettings;
    std::shared_ptr<JoyState> joyState;
    JoyState lastJoyState;
    Base::BaseControl::ControlLevel baseControlLevel;

    void checkChangeRelayButton(int buttonNumber);
    void checkChangeLightButton(int buttonNumber);

    int wheelRawProcess(int xAxis, int yAxis);
    int baseControlProcess(int xAxis, int yAxis);
    int moveCameraProcess(int xAxis, int yAxis);
public:
    ManualControl(std::shared_ptr<JoyState> joyState,
                  std::shared_ptr<Controls> controls,
                  std::mutex &grpcMutex,
                  Base::BaseControl::ControlLevel baseControlLevel = Base::BaseControl_ControlLevel_RAW);
    int update();
    void setBaseControlLevel(Base::BaseControl::ControlLevel newBaseControlLevel);
};

#endif // MOVEMENTS_H
