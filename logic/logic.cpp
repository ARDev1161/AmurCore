#include "logic.h"


Logic::Logic(JoyState *joyState, AMUR::AmurControls *controls, AMUR::AmurSensors *sensors):
    joyState(joyState),
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

    move->joyTranslate();
}

Mat Logic::getOutMat() const
{
    return *outMat;
}

void Logic::setOutMat(Mat &value)
{
    *outMat = value;
}
