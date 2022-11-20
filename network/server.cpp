#include "server.h"

void grpcServer::setProtosPointers(AMUR::AmurControls *const controls, AMUR::AmurSensors *const sensors)
{
    this->controls = controls;
    this->sensors = sensors;
}

grpc::Status grpcServer::DataExchange ([[maybe_unused]] grpc::ServerContext* context,
                          const AMUR::AmurSensors* request, AMUR::AmurControls* reply)
{
  std::unique_lock<std::mutex> ul(muServer);
  *sensors = *request;
  ul.unlock();

  *reply = *controls;

  return grpc::Status::OK;
}

grpc::Status grpcServer::DataStreamExchange ([[maybe_unused]] grpc::ServerContext* context,
                                grpc::ServerReaderWriter<AMUR::AmurControls, AMUR::AmurSensors >* stream)
{
    std::unique_lock<std::mutex> ul(muServer, std::defer_lock);

    while(true)
    {
      ul.lock();
      if(!(stream->Read(sensors)))
          return grpc::Status::OK;
      ul.unlock();

      // Write controls
      stream->Write(*controls);
    }
}

// TODO - make async methods
// TODO - add async broadcast message stream exchange
