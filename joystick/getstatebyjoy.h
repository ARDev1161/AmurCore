#ifndef GETSTATEBYJOY_H
#define GETSTATEBYJOY_H

#include <QVector>
#include <QTimer>
#include <QObject>
#include <QDebug>
#include "v_joystick_adapter.h"
#include "network/protobuf/amur.pb.h"
#include "joystickstate.h"

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
