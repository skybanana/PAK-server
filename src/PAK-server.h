#pragma once

#include <grpcpp/grpcpp.h>

#include <cstdint>
#include <memory>
#include <string>
#include <thread>

#include "echo.grpc.pb.h"

namespace grpc {
class Server;
class ServerCompletionQueue;
class ServerContext;
}  // namespace grpc

class PakServer final {
   public:
    PakServer();
    ~PakServer();
    void Run(uint16_t port);

   private:
    // 이벤트 처리하는 상태 객체 머신
    class CallData final {
       public:
        CallData(echo::EchoService::AsyncService* service, grpc::ServerCompletionQueue* cq);
        void Proceed();

       private:
        enum class CallStatus { CREATE, PROCESS, FINISH };

        echo::EchoService::AsyncService* service_;
        grpc::ServerCompletionQueue* cq_;

        grpc::ServerContext ctx_;
        echo::EchoRequest request_;
        echo::EchoReply reply_;

        grpc::ServerAsyncResponseWriter<echo::EchoReply> responder_;
        CallStatus status_;
    };

    void HandleRpcs();  // CQ 이벤트 루프
    std::unique_ptr<grpc::ServerCompletionQueue> cq_;
    echo::EchoService::AsyncService service_;
    std::unique_ptr<grpc::Server> server_;
};
