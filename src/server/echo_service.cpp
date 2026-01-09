#include "echo_service.h"

#include <boost/asio/use_awaitable.hpp>

EchoService::EchoService(agrpc::GrpcContext& grpc_context)
{
    RegisterHandlers(grpc_context);
}

echo::EchoService::AsyncService& EchoService::service()
{
    return service_;
}

void EchoService::RegisterHandlers(
    agrpc::GrpcContext& grpc_context)
{
    agrpc::register_awaitable_rpc_handler(
        grpc_context,
        service_,
        &echo::EchoService::AsyncService::RequestEcho,
        [](grpc::ServerContext&,
           echo::EchoRequest& request,
           grpc::ServerAsyncResponseWriter<echo::EchoResponse>& writer)
            -> net::awaitable<void>
        {
            echo::EchoResponse response;
            response.set_message(request.message());

            co_await agrpc::finish(
                writer,
                response,
                grpc::Status::OK,
                net::use_awaitable);
        });
}
