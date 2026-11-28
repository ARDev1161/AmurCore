#ifndef GETSTATEBYJOY_H
#define GETSTATEBYJOY_H

#include <QObject>
#include <QVector>
#include <QTimer>
#include <QDebug>
#include "v_joystick_adapter.h"
#include "joystick_factory.h"
#include <memory>
#include <mutex>
#include <chrono>
#include <cstdint>
#include "network/protobuf/robot.pb.h"

struct JoyState{
    int joyId = -1;
    std::shared_ptr<std::mutex> mutex = std::make_shared<std::mutex>();
    std::uint64_t generation = 0;
    bool connected = false;
    std::chrono::steady_clock::time_point sample_time{};

    QVector<bool> buttonVector; // Buttons state

    int joystickXaxis = 0;
    int joystickYaxis = 0;
    int joystickZLTaxis = 0;
    int joystickXrotation = 0;
    int joystickYrotation = 0;
    int joystickZRTaxis = 0;

    int joystickPOV0 = 0;
};


class GetStateByJoystick : public QObject
{
    Q_OBJECT

    std::unique_ptr<VJoystickAdapter> joyAdapter;
    enum { MAX_JOYSTICK_BUTTONS = 30 };

    int LoopTime=50;
    QTimer *pollTimer = nullptr;
    std::uint64_t generation_;

    std::shared_ptr<JoyState> joyState;
public:
    GetStateByJoystick(const std::shared_ptr<JoyState> state, std::uint64_t generation, QObject *parent = nullptr);
    ~GetStateByJoystick();
    void stop();

private slots:
    void poll();
private:
    void invalidate();
};

#endif // GETSTATEBYJOY_H
