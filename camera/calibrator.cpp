#include "calibrator.h"

Calibrator::Calibrator(CamSettingsHolder *holder)
{
    camHolder = holder;
}

Calibrator::~Calibrator()
{
    stopThreads();

    if (camHolder != nullptr)
        delete this->camHolder;
}

void Calibrator::addThread()
{
    CalibratorWorker* calibWorker = new CalibratorWorker(camHolder);
    QThread* calibThread = new QThread;
    calibWorker->moveToThread(calibThread);

    connect(calibThread, SIGNAL(started()), calibWorker, SLOT(process()));
    connect(calibWorker, SIGNAL(finished()), calibThread, SLOT(quit()));
    connect(this, SIGNAL(stopAll()), calibWorker, SLOT(stop()));
    connect(calibWorker, SIGNAL(finished()), calibWorker, SLOT(deleteLater()));
    connect(calibThread, SIGNAL(finished()), calibThread, SLOT(deleteLater()));

    calibThread->start();
}

void Calibrator::stopThreads()
{
    emit  stopAll();
}
