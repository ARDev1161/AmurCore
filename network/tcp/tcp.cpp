#include "tcp.h"

TCP::TCP(AMUR::AmurControls *controls, AMUR::AmurSensors *sensors, QString *hostname):
    controls(controls), sensors(sensors)
{
    hostName = hostname;
}

TCP::~TCP()
{
    stopThreads();
}

void TCP::addThread()
{
    TCPWorker* tcpWorker = new TCPWorker(controls, sensors, hostName);
    QThread* tcpThread = new QThread;
    tcpWorker->moveToThread(tcpThread);

    connect(tcpThread, SIGNAL(started()), tcpWorker, SLOT(process()));
    connect(tcpWorker, SIGNAL(finished()), tcpThread, SLOT(quit()));
    connect(this, SIGNAL(stopAll()), tcpWorker, SLOT(stop()));
    connect(tcpWorker, SIGNAL(finished()), tcpWorker, SLOT(deleteLater()));
    connect(tcpThread, SIGNAL(finished()), tcpThread, SLOT(deleteLater()));

    tcpThread->start();
}

void TCP::stopThreads()
{
    emit  stopAll();
}
