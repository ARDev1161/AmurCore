#include "client.h"

grpcClient::grpcClient(std::shared_ptr<grpc::Channel> channel, AMUR::AmurSensors* sensors, AMUR::AmurControls * const controls)
    : stub_(AMUR::ServerOnRobot::NewStub(channel))
{
    clientChannel = channel;
    this->controls = controls;
    this->sensors = sensors;
}

// Assembles the client's payload, sends it and presents the response back from the server.
grpc::Status grpcClient::DataExchange()
{
    // Container for the data we expect from the server.
    AMUR::AmurSensors reply;

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

    std::shared_ptr<grpc::ClientReaderWriter<AMUR::AmurControls, AMUR::AmurSensors> > stream(
        stub_->DataStreamExchange(&context));

    while(!stoppedStream && (clientChannel->GetState(true) == 2) )
    {
        // Write controls
        stream->Write(*controls);

        std::unique_lock<std::mutex> lock(muClient);

        // Read sensors & write to protos
        stream->Read(sensors);
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
