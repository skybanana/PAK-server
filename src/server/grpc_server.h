#pragma once

#include <string>
#include <memory>
#include <vector>
#include <thread>

#include <grpcpp/grpcpp.h>
#include <agrpc/asio_grpc.hpp>

class GrpcServer
{
public:
    explicit GrpcServer(std::string address);

    grpc::ServerBuilder& builder();
    agrpc::GrpcContext& grpc_context();

    void Run();
    void Shutdown();

private:
    std::string address_;

    grpc::ServerBuilder builder_;
    std::unique_ptr<grpc::Server> server_;
    std::unique_ptr<grpc::ServerCompletionQueue> cq_;
    std::unique_ptr<agrpc::GrpcContext> grpc_context_;

    std::vector<std::jthread> threads_;
};
