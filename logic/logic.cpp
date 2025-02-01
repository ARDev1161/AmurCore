#include "logic.h"


Logic::Logic(std::shared_ptr<JoyState> joyState,
             std::shared_ptr<Controls> controls,
             std::shared_ptr<Sensors> sensors)
    : joyState(joyState),
      controls(controls),
      sensors(sensors)
{
    initLogic();
}

void Logic::initLogic()
{
    srcMat = new Mat();
    outMat = new Mat();

    move = new Movements(joyState, controls);
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
