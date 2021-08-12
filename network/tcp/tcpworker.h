#ifndef TCPWORKER_H
#define TCPWORKER_H

#include "tcpcontrol.h"
#include "tcpsensors.h"


class TCPWorker: public QObject
{
    Q_OBJECT

    TCPControl *tcpControl;
    TCPSensors *tcpSensors;

    AMUR::AmurControls *controls;
    AMUR::AmurSensors *sensors;

    QString *hostName;

public:
    TCPWorker(AMUR::AmurControls *control, AMUR::AmurSensors *sensors, QString *hostname);
    ~TCPWorker();

public slots:
    void process();
    void stop();

signals:
    void finished();
};

#endif // TCPWORKER_H
