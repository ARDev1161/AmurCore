#ifndef JOYSTICK_H
#define JOYSTICK_H

#include "joystickstateworker.h"
#include "joystickdialog.h"

class Joystick: public QObject
{
    Q_OBJECT

    std::shared_ptr<JoyState> joyState;
public:
    Joystick(const std::shared_ptr<JoyState> state);
    ~Joystick();

    void addThread();
    void stopThreads();

signals:
    void stopAll(); //остановка всех потоков
};

#endif // JOYSTICK_H
