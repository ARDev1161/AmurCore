#ifndef LOGIC_H
#define LOGIC_H

#include "opencv2/opencv.hpp"
#include "movements.h"

using namespace Robot;

using namespace cv;
class Logic
{
    Mat *srcMat;
    Mat *outMat;

    std::shared_ptr<JoyState> joyState;
    Movements *move;

    std::shared_ptr<Controls> controls;
    std::shared_ptr<Sensors> sensors;
    std::shared_ptr<map_service::OccupancyGrid> map;

    void initLogic();
public:
    Logic(std::shared_ptr<JoyState> joyState, std::shared_ptr<Controls> controls, std::shared_ptr<Sensors> sensors);

    void process();

    Mat getOutMat() const;
    void setOutMat(Mat &value);

    void setSrcMat(Mat *const value);
};

#endif // LOGIC_H
