#include "tcpworker.h"

TCPWorker::TCPWorker(AmurControls *controls, AmurSensors *sensors, QString *hostname)
{
    amurControls = controls;
    amurSensors = sensors;
    hostName = hostname;
}

TCPWorker::~TCPWorker()
{
    if (tcpControl != nullptr)
        delete tcpControl;

    if (tcpSensors != nullptr)
        delete tcpSensors;
}

void TCPWorker::process()
{
    tcpControl = new TCPControl(amurControls, hostName);

    //tcpSensors = new TCPSensors(amurSensors, hostName);
}

void TCPWorker::stop()
{
    if(tcpControl != nullptr)
            tcpControl->stop();

    if(tcpSensors != nullptr)
            tcpSensors->stop();
}
