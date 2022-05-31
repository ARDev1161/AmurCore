#ifndef JOYSTICK_H
#define JOYSTICK_H

#include "joystickstateworker.h"
#include "joystickdialog.h"

class Joystick: public QObject
{
    Q_OBJECT

    JoyState *joyState;
public:
    Joystick(JoyState * const state);
    ~Joystick();

    void addThread();
    void stopThreads();

signals:
    void stopAll(); //остановка всех потоков
};

#endif // JOYSTICK_H
