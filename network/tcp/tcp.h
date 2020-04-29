#ifndef TCP_H
#define TCP_H

#include <QObject>
#include "tcpworker.h"

class TCP: public QObject
{
    Q_OBJECT

    AmurControls *amurControls;
    AmurSensors *amurSensors;

    QString *hostName;

public:
    TCP(AmurControls *controls, AmurSensors *sensors, QString *hostname);
    ~TCP();

    void addThread();
    void stopThreads();

signals:
    void stopAll(); //остановка всех потоков

};

#endif // TCP_H
