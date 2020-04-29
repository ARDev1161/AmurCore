#ifndef CAMSETTINGSHOLDER_H
#define CAMSETTINGSHOLDER_H

#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

class CamSettingsHolder
{
    bool ready = false;

    bool mustInitUndistort;

    int flag;

    Size imageSize;
    Mat map1, map2;    

    Mat distCoeffs;
    Mat cameraMatrix;

    vector<Mat> rvecs;
    vector<Mat> tvecs;

    vector<vector<Point3f>> objectPoints; // real world 3D coordinates
    vector<vector<Point2f>> imagePoints; // 2D coordinates (pixel values)

public:
    CamSettingsHolder();

    Mat remap(const Mat &image);

    bool getReady() const;
    void setReady(bool value);

    Mat getCameraMatrix() const;
    void setCameraMatrix(const Mat &value);
    Mat getDistCoeffs() const;
    void setDistCoeffs(const Mat &value);

    vector<vector<Point3f> > getObjectPoints() const;
    void setObjectPoints(const vector<vector<Point3f> > &value);
    vector<vector<Point2f> > getImagePoints() const;
    void setImagePoints(const vector<vector<Point2f> > &value);

    vector<Mat> getRvecs() const;
    void setRvecs(const vector<Mat> &value);
    vector<Mat> getTvecs() const;
    void setTvecs(const vector<Mat> &value);

    int getFlag() const;
    void setFlag(int value);
    Size getImageSize() const;
    void setImageSize(const Size &value);
};

#endif // CAMSETTINGSHOLDER_H
