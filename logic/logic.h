#ifndef LOGIC_H
#define LOGIC_H

#include "opencv2/opencv.hpp"
#include "speech/speechdialog.h"
#include "movements.h"

using namespace cv;
class Logic
{
    Mat *srcMat;
    Mat *outMat;

    JoyState *joyState;
    Movements *move;

    AMUR::AmurControls *controls;
    AMUR::AmurSensors *sensors;

    QString keyboardConfigPath = "/home/ardev/projects/Amur/src/AmurCore/logic/bindings/keyboard.json";
    QString joyConfigPath = "/home/ardev/projects/Amur/src/AmurCore/logic/bindings/joystick/dualsense.json";

    void initLogic();
public:
    Logic(JoyState *joyState, AMUR::AmurControls *controls, AMUR::AmurSensors *sensors);

    void process();

    Mat getOutMat() const;
    void setOutMat(Mat &value);

    void setSrcMat(Mat *const value);
};

#endif // LOGIC_H
