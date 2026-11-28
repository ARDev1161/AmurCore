#include "logic.h"


Logic::Logic(std::shared_ptr<JoyState> joyState,
             std::shared_ptr<Controls> controls,
             std::shared_ptr<Sensors> sensors,
             std::mutex &grpcMutex, TeleopIntent &intent, int deadmanButton)
    : joyState(joyState),
      controls(controls),
      sensors(sensors),
      grpcMutex_(grpcMutex), intent_(intent), deadmanButton_(deadmanButton)
{
    initLogic();
}

void Logic::initLogic()
{
    srcMat = new Mat();
    outMat = new Mat();

    move = new ManualControl(joyState, controls, sensors, grpcMutex_, intent_, deadmanButton_);
}

void Logic::setSrcMat(Mat *const value)
{
    srcMat = value;
}

void Logic::process()
{
//    srcMat->copyTo(outMat);
    outMat = srcMat;

    move->update();
}

Mat Logic::getOutMat() const
{
    return *outMat;
}

void Logic::setOutMat(Mat &value)
{
    *outMat = value;
}
