#ifndef GETSTATEBYJOY_H
#define GETSTATEBYJOY_H

#include <QVector>
#include <QTimer>
#include <QObject>
#include <QDebug>
#include "v_joystick_adapter.h"
#include "network/protobuf/amur.pb.h"

struct JoyState{
    int joyId = -1;

    QVector<bool> buttonVector; // Buttons state

    int joystickXaxis = 0;
    int joystickYaxis = 0;
    int joystickZLTaxis = 0;
    int joystickXrotation = 0;
    int joystickYrotation = 0;
    int joystickZRTaxis = 0;

    int joystickPOV0 = 0;
};


class GetStateByJoystick : public QObject
{
    Q_OBJECT

    VJoystickAdapter* joyAdapter;

    enum { MAX_JOYSTICK_BUTTONS = 30 };

    int LoopTime=50;

    bool loopFlag;

    JoyState *joyState;
public:
    GetStateByJoystick(int id, JoyState * const state, QObject *parent = nullptr);
    ~GetStateByJoystick();
    void stop();

private:
    void setAvaliableJoystick();

private slots:
    void connectToJoystick();
    void disconnectFromJoystick();

    void axisSetup(int id, int state);
    void hatSetup(int id, int state);
    void buttonSetup(int id, bool state);
    void ballSetup(int id, int stateX, int stateY);
};

#endif // GETSTATEBYJOY_H
