#include "PAK-server.h"

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>

#include <iostream>

ABSL_FLAG(std::string, target, "localhost:50051", "Server address");

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;

PakServer::PakServer() = default;

PakServer::~PakServer() {
    if (server_)
        server_->Shutdown();
    if (cq_)
        cq_->Shutdown();
}

void PakServer::Run(uint16_t port) {
    const std::string server_address = absl::StrFormat("0.0.0.0:%d", port);

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service_);
    cq_ = builder.AddCompletionQueue();

    server_ = builder.BuildAndStart();
    std::cout << "sever listening on" << server_address << "\n";

    HandleRpcs();
}

void PakServer::HandleRpcs() {
    new CallData(&service_, cq_.get());

    void* tag;
    bool ok;

    while (true) {
        const bool alive = cq_->Next(&tag, &ok);
        if (!alive || !ok)
            break;
        static_cast<CallData*>(tag)->Proceed();
    }
}

PakServer::CallData::CallData(echo::EchoService::AsyncService* service,
                              grpc::ServerCompletionQueue* cq)
    : service_(service), cq_(cq), responder_(&ctx_), status_(CallStatus::CREATE) {
    Proceed();
}

void PakServer::CallData::Proceed() {
    switch (status_) {
        case CallStatus::CREATE:
            status_ = CallStatus::PROCESS;
            service_->RequestEcho(&ctx_, &request_, &responder_, cq_, cq_, this);
            break;
        case CallStatus::PROCESS:
            new CallData(service_, cq_);

            reply_.set_message(request_.message());

            status_ = CallStatus::FINISH;
            responder_.Finish(reply_, Status::OK, this);
            break;
        case CallStatus::FINISH:
            delete this;
            break;
        default:
            std::exit(0);
    }
}

ABSL_FLAG(uint16_t, port, 50051, "Server port for the service");

int main(int argc, char** argv) {
    std::cout << "start" << "\n";
    absl::ParseCommandLine(argc, argv);
    PakServer server;
    server.Run(absl::GetFlag(FLAGS_port));
    return 0;
}
