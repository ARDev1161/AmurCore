#ifndef CALIBRATORWORKER_H
#define CALIBRATORWORKER_H

#include <QObject>
#include "camsettingsholder.h"

class CalibratorWorker : public QObject
{
    Q_OBJECT

    CamSettingsHolder *camHolder;

    Mat distCoeffs;
    Mat cameraMatrix;

    vector<Mat> rvecs;
    vector<Mat> tvecs;

public:
    CalibratorWorker(CamSettingsHolder *holder);
    ~CalibratorWorker();

public slots:
    void process();
    void stop();

signals:
    void finished();
};

#endif // CALIBRATORWORKER_H
