#ifndef JOYSTICKSTATEWORKER_H
#define JOYSTICKSTATEWORKER_H

#include "getstatebyjoy.h"

class JoystickStateWorker : public QObject
{
    Q_OBJECT

    std::shared_ptr<JoyState> joyState;
    GetStateByJoystick *joyStateController;
public:
    JoystickStateWorker(const std::shared_ptr<JoyState> state);
    ~JoystickStateWorker();

public slots:
    void process();
    void stop();

signals:
    void finished();
};

#endif // JOYSTICKSTATEWORKER_H
