#include "camsettingsholder.h"

CamSettingsHolder::CamSettingsHolder()
{
    ready = false;
}

// remove distortion in the image (after calibration)
Mat CamSettingsHolder::remap(const Mat &image)
{
    Mat undistorted;
    if (ready){
        if (mustInitUndistort) { // called once per calibration
            initUndistortRectifyMap(
                            cameraMatrix,  // computed camera matrix
                            distCoeffs, // computed distortion matrix
                            Mat(), // optional rectification (none)
                            Mat(), // camera matrix to generate undistorted
                            imageSize,  // size of undistorted
                            CV_32FC1,      // type of output map
                            map1, map2);   // the x and y mapping functions
            mustInitUndistort = false;
        }

    // apply mapping functions
    cv::remap(image, undistorted, map1, map2, INTER_LINEAR); // interpolation type
    }

    return undistorted;
}

vector<vector<Point3f> > CamSettingsHolder::getObjectPoints() const
{
    return objectPoints;
}

void CamSettingsHolder::setObjectPoints(const vector<vector<Point3f> > &value)
{
    objectPoints = value;
}

vector<vector<Point2f> > CamSettingsHolder::getImagePoints() const
{
    return imagePoints;
}

void CamSettingsHolder::setImagePoints(const vector<vector<Point2f> > &value)
{
    imagePoints = value;
}

vector<Mat> CamSettingsHolder::getRvecs() const
{
    return rvecs;
}

void CamSettingsHolder::setRvecs(const vector<Mat> &value)
{
    rvecs = value;
}

vector<Mat> CamSettingsHolder::getTvecs() const
{
    return tvecs;
}

void CamSettingsHolder::setTvecs(const vector<Mat> &value)
{
    tvecs = value;
}

int CamSettingsHolder::getFlag() const
{
    return flag;
}

void CamSettingsHolder::setFlag(int value)
{
    flag = value;
}

Size CamSettingsHolder::getImageSize() const
{
    return imageSize;
}

void CamSettingsHolder::setImageSize(const Size &value)
{
    imageSize = value;
}

Mat CamSettingsHolder::getCameraMatrix() const
{
    return cameraMatrix;
}

void CamSettingsHolder::setCameraMatrix(const Mat &value)
{
    cameraMatrix = value;
}

Mat CamSettingsHolder::getDistCoeffs() const
{
    return distCoeffs;
}

void CamSettingsHolder::setDistCoeffs(const Mat &value)
{
    distCoeffs = value;
}

bool CamSettingsHolder::getReady() const
{
    return ready;
}

void CamSettingsHolder::setReady(bool value)
{
    ready = value;
}
