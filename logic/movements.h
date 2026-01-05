#ifndef MOVEMENTS_H
#define MOVEMENTS_H

#include "../joystick/joystick.h"
#include <memory>
#include <utility>
#include <stdlib.h>     /* abs */

using namespace Robot;

struct MoveSettings
{
    struct JoyBindings
    {
        int relayButton = 5;
        int lightButton = 0;

        int divider = 129;

        int wheelXAxis(const JoyState &state) const { return state.joystickXaxis / divider; }
        int wheelYAxis(const JoyState &state) const { return state.joystickYaxis / divider; }

        int cameraXAxis(const JoyState &state) const { return state.joystickZLTaxis / divider; }
        int cameraYAxis(const JoyState &state) const { return state.joystickXrotation / divider; }
    }
    joyBindings;
};

class BaseControlStrategy
{
public:
    virtual ~BaseControlStrategy() = default;
    virtual std::pair<double, double> velocities(const JoyState &state,
                                                 const MoveSettings::JoyBindings &bindings) const = 0;
};

class DeadzoneBaseControlStrategy : public BaseControlStrategy
{
public:
    explicit DeadzoneBaseControlStrategy(int driftZone) : driftZone_(driftZone) {}
    std::pair<double, double> velocities(const JoyState &state,
                                         const MoveSettings::JoyBindings &bindings) const override;

private:
    int driftZone_;
};

class ManualControl
{
    std::shared_ptr<Controls> controls;
    std::mutex &grpcMutex_;

    MoveSettings moveSettings;
    std::shared_ptr<JoyState> joyState;
    JoyState lastJoyState;
    int joyDriftZone = 7;

    std::unique_ptr<BaseControlStrategy> baseControlStrategy;

    Base::BaseControl::ControlLevel baseControlLevel;

    void checkChangeRelayButton(int buttonNumber);
    void checkChangeLightButton(int buttonNumber);

    int wheelRawProcess(int xAxis, int yAxis);
    int baseControlProcess(int xAxis, int yAxis);
    int moveCameraProcess(int xAxis, int yAxis);
public:
    ManualControl(std::shared_ptr<JoyState> joyState,
                  std::shared_ptr<Controls> controls,
                  std::mutex &grpcMutex,
                  Base::BaseControl::ControlLevel baseControlLevel = Base::BaseControl_ControlLevel_RAW);
    int update();
    void setBaseControlLevel(Base::BaseControl::ControlLevel newBaseControlLevel);
};

#endif // MOVEMENTS_H
