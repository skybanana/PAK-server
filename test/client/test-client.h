#pragma once

#include <grpcpp/grpcpp.h>

#include <memory>
#include <string>

#include "echo.grpc.pb.h"

class TestClient {
   public:
    explicit TestClient(std::shared_ptr<grpc::Channel> channel);
    std::string echo(const std::string& message);

   private:
    std::unique_ptr<echo::EchoService::Stub> stub_;
};