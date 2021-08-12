#ifndef JOYSTICKSTATE_H
#define JOYSTICKSTATE_H

#include <QVector>

struct JoyState{

    int joyId;

    QVector<bool> buttonVector; // Buttons state

    int joystickXaxis = 0;
    int joystickYaxis = 0;
    int joystickZLTaxis = 0;
    int joystickXrotation = 0;
    int joystickYrotation = 0;
    int joystickZRTaxis = 0;

    int joystickPOV0 = 0;
};

#endif // JOYSTICKSTATE_H
