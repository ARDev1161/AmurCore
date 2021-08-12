#ifndef TCP_H
#define TCP_H

#include <QObject>
#include "tcpworker.h"

class TCP: public QObject
{
    Q_OBJECT

    AMUR::AmurControls *controls;
    AMUR::AmurSensors *sensors;

    QString *hostName;

public:
    TCP(AMUR::AmurControls *controls, AMUR::AmurSensors *sensors, QString *hostname);
    ~TCP();

    void addThread();
    void stopThreads();

signals:
    void stopAll(); //остановка всех потоков

};

#endif // TCP_H
