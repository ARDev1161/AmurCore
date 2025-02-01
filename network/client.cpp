#include "client.h"

grpcClient::grpcClient(std::shared_ptr<grpc::Channel> channel,
               std::shared_ptr<Controls> controlsPtr,
               std::shared_ptr<Sensors> sensorsPtr,
               std::shared_ptr<map_service::GetMapResponse> mapPtr)
      : stub_(ServerOnRobot::NewStub(channel)),
        controls(controlsPtr),
        sensors(sensorsPtr),
        map(mapPtr)
{
    clientChannel = channel;
}

// Assembles the client's payload, sends it and presents the response back from the server.
grpc::Status grpcClient::DataExchange()
{
    // Container for the data we expect from the server.
    Sensors reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    grpc::ClientContext context;

    // The actual RPC.
    grpc::Status status = stub_->DataExchange(&context, *controls, &reply);

    std::unique_lock<std::mutex> ul(muClient);

    // Act upon its status.
    if (status.ok())
      *sensors = reply;
    else
      std::cout << "DataExchange rpc failed!" << std::endl;

    return status;
}

void grpcClient::stopStream()
{
    stoppedStream = true;
}

grpc::Status grpcClient::DataStreamExchange()
{
    stoppedStream = false;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    grpc::ClientContext context;

    std::shared_ptr<grpc::ClientReaderWriter<Controls, Sensors> > stream(
        stub_->DataStreamExchange(&context));

    while(!stoppedStream && (clientChannel->GetState(true) == 2) )
    {
        // Write controls
        stream->Write(*controls);

        std::unique_lock<std::mutex> lock(muClient);

        // Read sensors & write to protos
        stream->Read(sensors.get());
    }

    stream->WritesDone();

    grpc::Status status = stream->Finish();
    if (!status.ok()) {
      std::cout << "Error " << status.error_code() << " : " << status.error_message() << std::endl;
      std::cout << "DataStreamExchange rpc failed." << std::endl;
    }

    stoppedStream = false;

    return status;
}

grpc::Status grpcClient::MapStream()
{
    // Container for the data we expect from the server.
    map_service::GetMapRequest request;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    grpc::ClientContext context;

    std::shared_ptr<grpc::ClientReaderWriter<map_service::GetMapRequest, map_service::GetMapResponse> > stream(
        stub_->MapStream(&context));

    while(!stoppedStream && (clientChannel->GetState(true) == 2) )
    {
        // Request map
        stream->Write(request);

        std::unique_lock<std::mutex> lock(muClient);

        // Read map
        stream->Read(map.get());
    }

    stream->WritesDone();

    grpc::Status status = stream->Finish();
    if (!status.ok()) {
      std::cout << "Error " << status.error_code() << " : " << status.error_message() << std::endl;
      std::cout << "DataStreamExchange rpc failed." << std::endl;
    }

    stoppedStream = false;

    return status;
}
