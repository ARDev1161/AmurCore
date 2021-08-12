#include "tcpworker.h"

TCPWorker::TCPWorker(AMUR::AmurControls *controls, AMUR::AmurSensors *sensors, QString *hostname):
    controls(controls), sensors(sensors), hostName(hostname)
{
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
    tcpControl = new TCPControl(controls, hostName);

    //tcpSensors = new TCPSensors(sensors, hostName);
}

void TCPWorker::stop()
{
    if(tcpControl != nullptr)
            tcpControl->stop();

    if(tcpSensors != nullptr)
            tcpSensors->stop();
}
