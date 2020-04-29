#include "logic.h"


Logic::Logic()
{
    initLogic();
}

void Logic::initLogic()
{
    srcMat = new Mat();
    outMat = new Mat();
}

void Logic::setSrcMat(Mat *const value)
{
    srcMat = value;
    logicProcess();
}

void Logic::logicProcess()
{
//    srcMat->copyTo(outMat);
    outMat = srcMat;
}

Mat Logic::getOutMat() const
{
    return *outMat;
}

void Logic::setOutMat(Mat &value)
{
    *outMat = value;
}
