#ifndef JOYSTICKSTATEWORKER_H
#define JOYSTICKSTATEWORKER_H

#include "getstatebyjoy.h"

class JoystickStateWorker : public QObject
{
    Q_OBJECT

    JoyState *joyState;
    GetStateByJoystick *joyStateController;
public:
    JoystickStateWorker(JoyState * const state);
    ~JoystickStateWorker();

public slots:
    void process();
    void stop();

signals:
    void finished();
};

#endif // JOYSTICKSTATEWORKER_H
