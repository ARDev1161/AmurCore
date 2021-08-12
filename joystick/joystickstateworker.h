#ifndef JOYSTICKSTATEWORKER_H
#define JOYSTICKSTATEWORKER_H

#include "getstatebyjoy.h"


class JoystickStateWorker : public QObject
{
    Q_OBJECT

    int joyId = -1;
    JoyState *joyState;
    GetStateByJoystick *joyStateController;

    void changeJoystickId(int id);

public:
    JoystickStateWorker(int id, JoyState * const state);
    ~JoystickStateWorker();

public slots:
    void process();
    void stop();

signals:
    void finished();
};

#endif // JOYSTICKSTATEWORKER_H
