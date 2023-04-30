#ifndef CALIBRATOR_H
#define CALIBRATOR_H

#include <QObject>
#include <QThread>
#include "calibratorworker.h"

class Calibrator : public QObject
{
    Q_OBJECT

    CamSettingsHolder *camHolder;

public:
    Calibrator(CamSettingsHolder *holder);
    ~Calibrator();

    void addThread();
    void stopThreads();

signals:
    void stopAll(); //остановка всех потоков

};

#endif // CALIBRATOR_H
