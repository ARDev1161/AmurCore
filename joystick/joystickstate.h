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

//amurControls->mutable_light()->set_ledleftpower(0);
//amurControls->mutable_light()->set_ledrightpower(0);

//amurControls->mutable_wheelmotors()->set_lefttime(0);
//amurControls->mutable_wheelmotors()->set_leftpower(0);
//amurControls->mutable_wheelmotors()->set_righttime(0);
//amurControls->mutable_wheelmotors()->set_rightpower(0);

//amurControls->mutable_handmotors()->set_lefttime(0);
//amurControls->mutable_handmotors()->set_leftpower(0);
//amurControls->mutable_handmotors()->set_righttime(0);
//amurControls->mutable_handmotors()->set_rightpower(0);

//amurControls->mutable_handmotors()->set_leftrelay(false);
//amurControls->mutable_handmotors()->set_rightrelay(false);

//amurControls->mutable_cameraservos()->set_xangle(joyState.joystickXaxis/364);
//amurControls->mutable_cameraservos()->set_yangle(joyState.joystickYaxis/364);

//amurControls->mutable_system()->set_haltflag(false);
//amurControls->mutable_system()->set_restartflag(false);
