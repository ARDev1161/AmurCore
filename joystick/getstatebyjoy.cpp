#include "getstatebyjoy.h"

GetStateByJoystick::GetStateByJoystick(JoyState * const state, QObject *parent) :
    QObject(parent),
    loopFlag(true),
    joyState(state)
{
    joyState->buttonVector = QVector<bool>(MAX_JOYSTICK_BUTTONS);
    joyAdapter = new VJoystickAdapter();
    connectToJoystick();
}

GetStateByJoystick::~GetStateByJoystick()
{
    disconnectFromJoystick();
}

void GetStateByJoystick::stop()
{
    loopFlag = false;
}

void GetStateByJoystick::connectToJoystick()
{
    if(joyState->joyId != -1)
        if(joyAdapter->open(joyState->joyId))
        {
            connect(joyAdapter, SIGNAL(sigButtonChanged(int,bool)), this, SLOT(buttonSetup(int,bool)));
            connect(joyAdapter, SIGNAL(sigAxisChanged(int,int)), this, SLOT(axisSetup(int,int)));
            connect(joyAdapter, SIGNAL(sigHatChanged(int,int)), this, SLOT(hatSetup(int,int)));
            connect(joyAdapter, SIGNAL(sigBallChanged(int,int,int)), this, SLOT(ballSetup(int,int,int)));
        }
}

void GetStateByJoystick::disconnectFromJoystick()
{
    if(joyAdapter->isConnected())
    {
        joyAdapter->close();
        disconnect(joyAdapter, SIGNAL(sigButtonChanged(int,bool)), this, SLOT(buttonSetup(int,bool)));
        disconnect(joyAdapter, SIGNAL(sigAxisChanged(int,int)), this, SLOT(axisSetup(int,int)));
        disconnect(joyAdapter, SIGNAL(sigHatChanged(int,int)), this, SLOT(hatSetup(int,int)));
        disconnect(joyAdapter, SIGNAL(sigBallChanged(int,int,int)), this, SLOT(ballSetup(int,int,int)));
    }
    joyState->joyId = -1;
}

void GetStateByJoystick::axisSetup(int id, int state)
{
    switch(id)
    {
    case 0:
        joyState->joystickXaxis = (tr("%1").arg(state)).toInt();
        break;
    case 1:
        joyState->joystickYaxis = (tr("%1").arg(-1*state)).toInt();
        break;
    case 2:
        joyState->joystickZLTaxis = (tr("%1").arg(state)).toInt();
        break;
    case 3:
        joyState->joystickXrotation = (tr("%1").arg(state)).toInt();
        break;
    case 4:
        joyState->joystickYrotation = (tr("%1").arg(-1*state)).toInt();
        break;
    case 5:
        joyState->joystickZRTaxis = (tr("%1").arg(state)).toInt();
        break;
    }
}

void GetStateByJoystick::hatSetup(int id, int state)
{
    Q_UNUSED(id);
    joyState->joystickPOV0 = (tr("%1").arg(state)).toInt();
}

void GetStateByJoystick::buttonSetup(int id, bool state)
{
    joyState->buttonVector[id] = state;

    QString buttonTest = "";
    for(int i = 0; i < MAX_JOYSTICK_BUTTONS; ++i)
    {
        if(joyState->buttonVector[i] == true)
        {
            if(i < 10)
                buttonTest += tr("0%1  ").arg(i);
            else
                buttonTest += tr("%1").arg(i);
        }
    }
}

void GetStateByJoystick::ballSetup(int id, int stateX, int stateY)
{
    Q_UNUSED(id);
    Q_UNUSED(stateX);
    Q_UNUSED(stateY);
}
