#include "grpc_server.h"

#include <iostream>
#include <algorithm>

GrpcServer::GrpcServer(std::string address)
    : address_(std::move(address))
{
    builder_.AddListeningPort(
        address_, grpc::InsecureServerCredentials());

    cq_ = builder_.AddCompletionQueue();
    grpc_context_ =
        std::make_unique<agrpc::GrpcContext>(*cq_);
}

grpc::ServerBuilder& GrpcServer::builder()
{
    return builder_;
}

agrpc::GrpcContext& GrpcServer::grpc_context()
{
    return *grpc_context_;
}

void GrpcServer::Run()
{
    server_ = builder_.BuildAndStart();

    const int thread_count =
        std::max(2u, std::thread::hardware_concurrency());

    std::cout << "[GrpcServer] Listening on "
              << address_ << "\n";

    for (int i = 0; i < thread_count; ++i)
    {
        threads_.emplace_back([this] {
            grpc_context_->run();
        });
    }

    grpc_context_->run();
}

void GrpcServer::Shutdown()
{
    if (server_)
        server_->Shutdown();

    if (grpc_context_)
        grpc_context_->stop();
}
