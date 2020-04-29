#include "session.h"

Session::Session()
{

}

Session::~Session()
{
    stopThreads();
}

void Session::addThread()
{

}

void Session::stopThreads()
{
    emit  stopAll();
}
