#ifndef JOYSTICK_H
#define JOYSTICK_H

#include "joystickstateworker.h"
#include "joystickdialog.h"

class Joystick: public QObject
{
    Q_OBJECT

    int joyId;

    JoyState *joyState;

    AmurControls *amurControl;

public:
    Joystick(int id, AmurControls * const controls, JoyState * const state);
    ~Joystick();

    void addThread();
    void stopThreads();

signals:
    void stopAll(); //остановка всех потоков
};

#endif // JOYSTICK_H
