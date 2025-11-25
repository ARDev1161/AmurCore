#include "worker.h"

WorkerBase::WorkerBase()
{
}

WorkerBase::~WorkerBase()
{
    stop();
}

void WorkerBase::process()
{
    onStart();
    if(running)
        onTick();
    onStop();
    emit finished();
}

void WorkerBase::stop()
{
    running = false;
}
