#ifndef CAMCALIBRATE_H
#define CAMCALIBRATE_H

#include <QDialog>
#include "camsettingsholder.h"
#include "calibrator.h"

#define COUNT_SKIP_FRAMES 7

using namespace cv;
using namespace std;

namespace Ui {
class CamCalibrate;
}

class CamCalibrate : public QDialog
{
    Q_OBJECT

    Ui::CamCalibrate *ui;

    bool started = false;

    int pauseForChangePosition = 0;

    int numCornersHor;
    int numCornersVer;
    int numSquares;

    int successes = 0;

    CamSettingsHolder *camHolder;
    Calibrator *camCalib;

    //input points:
    vector<Point3f> objectCorners;
    vector<Point2f> imageCorners;

    vector<vector<Point3f>> objectPoints; // real world 3D coordinates
    vector<vector<Point2f>> imagePoints; // 2D coordinates (pixel values)

    Size boardSize;

    Mat *sourceMat;

    Mat graySaved;
    Mat gray;
    Mat out;

    Mat distCoeffs;


public:
    explicit CamCalibrate(Mat *source, CamSettingsHolder *holder, QWidget *parent = 0);
    ~CamCalibrate();

public slots:
    void frameUpdate();

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
    void on_pushButton_clicked();


private:
    void outMat(Mat &ToOut);
    void initFields();
    void getCalibInfo();
};

#endif // CAMCALIBRATE_H
