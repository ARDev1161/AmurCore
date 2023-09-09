#include "getstatebyjoy.h"

GetStateByJoystick::GetStateByJoystick(JoyState * const state, QObject *parent) :
    QObject(parent),
    loopFlag(true),
    joyState(state)
{
    joyState->setButtonVector(QVector<bool>(MAX_JOYSTICK_BUTTONS));
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
    if(joyState->getJoyId() != -1)
        if(joyAdapter->open(joyState->getJoyId()))
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
    joyState->setJoyId(-1);
}

void GetStateByJoystick::axisSetup(int id, int state)
{
    switch(id)
    {
    case 0:
        joyState->setJoystickXaxis((tr("%1").arg(state)).toInt());
        break;
    case 1:
        joyState->setJoystickYaxis((tr("%1").arg(-1*state)).toInt());
        break;
    case 2:
        joyState->setJoystickZLTaxis((tr("%1").arg(state)).toInt());
        break;
    case 3:
        joyState->setJoystickXrotation((tr("%1").arg(state)).toInt());
        break;
    case 4:
        joyState->setJoystickYrotation((tr("%1").arg(-1*state)).toInt());
        break;
    case 5:
        joyState->setJoystickZRTaxis((tr("%1").arg(state)).toInt());
        break;
    }
}

void GetStateByJoystick::hatSetup(int id, int state)
{
    Q_UNUSED(id);
    joyState->setJoystickPOV0((tr("%1").arg(state)).toInt());
}

void GetStateByJoystick::buttonSetup(int id, bool state)
{
    joyState->setButton(id, state);

    QString buttonTest = "";
    for(int i = 0; i < MAX_JOYSTICK_BUTTONS; ++i)
    {
        if(joyState->getButton(i) == true)
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
