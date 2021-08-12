#include "joystick.h"

Joystick::Joystick(int id, AMUR::AmurControls * const controls, JoyState * const state):
    joyId(id),
    joyState(state),
    controls(controls)
{

}

Joystick::~Joystick()
{
    stopThreads();
}

void Joystick::addThread()
{
    JoystickStateWorker* joyWorker = new JoystickStateWorker(joyId, joyState);
    QThread* joyThread = new QThread;
    joyWorker->moveToThread(joyThread);

    connect(joyThread, SIGNAL(started()), joyWorker, SLOT(process()));
    connect(joyWorker, SIGNAL(finished()), joyThread, SLOT(quit()));
    connect(this, SIGNAL(stopAll()), joyWorker, SLOT(stop()));
    connect(joyWorker, SIGNAL(finished()), joyWorker, SLOT(deleteLater()));
    connect(joyThread, SIGNAL(finished()), joyThread, SLOT(deleteLater()));

    joyThread->start();
}

void Joystick::stopThreads()
{
    emit  stopAll();
}
