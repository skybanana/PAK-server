#pragma once

#include <grpcpp/grpcpp.h>

#include <agrpc/asio_grpc.hpp>
#include <agrpc/register_awaitable_rpc_handler.hpp>
#include <boost/asio/awaitable.hpp>

#include "out/build/ninja-debug/generated/echo.grpc.pb.h"


namespace asio = boost::asio;

class EchoService {
   public:
    explicit EchoService(agrpc::GrpcContext& grpc_context);
    echo::EchoService::AsyncService& service();

   private:
    echo::EchoService::AsyncService service_;
    void RegisterHandlers(agrpc::GrpcContext& grpc_context);
};
