#ifndef MOVEMENTS_H
#define MOVEMENTS_H

#include "../joystick/joystick.h"
#include "../network/teleop_intent.h"
#include <memory>

using namespace Robot;

class ManualControl
{
    std::shared_ptr<Controls> controls;
    std::shared_ptr<Sensors> sensors;
    std::mutex &grpcMutex_;
    TeleopIntent &intent_;
    std::shared_ptr<JoyState> joyState;
    JoyState lastJoyState;
    int deadmanButton_;
    bool releaseSeen_ = false;
    std::string robotId_;

    void checkChangeRelayButton(const JoyState &state, int buttonNumber);
    void checkChangeLightButton(const JoyState &state, int buttonNumber);
    int moveCameraProcess(int xAxis, int yAxis);
    void baseControlProcess(const JoyState &state);
public:
    ManualControl(std::shared_ptr<JoyState> joyState,
                  std::shared_ptr<Controls> controls,
                  std::shared_ptr<Sensors> sensors,
                  std::mutex &grpcMutex, TeleopIntent &intent, int deadmanButton);
    int update();
};

#endif // MOVEMENTS_H
