#ifndef MOVEMENTS_H
#define MOVEMENTS_H

#include <QShortcut>
#include <QSettings>
#include "../joystick/joystate.h"
#include "../network/protobuf/amur.pb.h"

struct BindingsConfig
{
    struct JoystickConfig
    {
        QString config = "joystick.cfg";
        int relayButton = 5;
        int lightButton = 0;

        #define WHEEL_X_AXIS joyState->getJoystickXaxis()
        #define WHEEL_Y_AXIS joyState->getJoystickYaxis()

        #define CAM_X_AXIS joyState->getJoystickZLTaxis()
        #define CAM_Y_AXIS joyState->getJoystickXrotation()

        #define HAND_AXIS joyState->getJoystickXrotation()

        #define DIVIDER 129
    }
    joyBindings;

    struct KeyboardConfig{
        QString config = "keyboard.cfg";
        int relayButton = 5;
        int lightButton = 0;
    }
    keyboardBindings;
};

class Movements
{
    AMUR::AmurControls *controls;
    BindingsConfig bindings;
    JoyState *joyState;
    JoyState lastJoyState;

    void checkChangeRelayButton(int buttonNumber);
    void checkChangeLightButton(int buttonNumber);

    int wheelProcess(int xAxis, int yAxis);
    int moveCamera(int xAxis, int yAxis);
public:
    Movements(JoyState *joyState, AMUR::AmurControls *controls);
    int update();
};

#endif // MOVEMENTS_H
