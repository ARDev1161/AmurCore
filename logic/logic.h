#ifndef LOGIC_H
#define LOGIC_H

#include "opencv2/opencv.hpp"
#include "logic/speech/speechdialog.h"

using namespace cv;
class Logic
{
    Mat *srcMat;
    Mat *outMat;

    void initLogic();
    void logicProcess();
public:
    Logic();

    Mat getOutMat() const;
    void setOutMat(Mat &value);

    void setSrcMat(Mat *const value);
};

#endif // LOGIC_H
