#include "calibratorworker.h"

CalibratorWorker::CalibratorWorker(CamSettingsHolder *holder)
{
    camHolder = holder;
}

CalibratorWorker::~CalibratorWorker()
{

    if (camHolder != nullptr)
        delete this->camHolder;

}

void CalibratorWorker::process()
{
    if (camHolder != nullptr){
        if(!camHolder->getImagePoints().empty())
            calibrateCamera(camHolder->getObjectPoints(),   // the 3D points
                            camHolder->getImagePoints(),    // the image points
                            camHolder->getImageSize(),      // image size
                            cameraMatrix,                 // output camera matrix
                            distCoeffs,                   // output distortion matrix
                            rvecs, tvecs,                   // Ts, Rs
                            camHolder->getFlag());          // set options

        camHolder->setCameraMatrix(cameraMatrix);
        camHolder->setDistCoeffs(distCoeffs);
        camHolder->setRvecs(rvecs);
        camHolder->setTvecs(tvecs);

        if (!camHolder->getCameraMatrix().empty())
            camHolder->setReady(true);
    }
}

void CalibratorWorker::stop()
{

}
