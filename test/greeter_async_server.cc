/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include <grpcpp/grpcpp.h>

#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/check.h"
#include "absl/strings/str_format.h"

#ifdef BAZEL_BUILD
#include "examples/protos/helloworld.grpc.pb.h"
#else
#include "helloworld.grpc.pb.h"
#endif

// 서비스가 사용할 서버 포트 (기본값: 50051)
ABSL_FLAG(uint16_t, port, 50051, "Server port for the service");

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;
using helloworld::Greeter;
using helloworld::HelloReply;
using helloworld::HelloRequest;

// 비동기 gRPC 서버의 실제 구현 클래스
class ServerImpl final {
   public:
    // 객체 소멸자
    ~ServerImpl() {
        server_->Shutdown();
        // 서버 종료 이후에 completion queue를 반드시 종료해야 함
        cq_->Shutdown();
    }

    // 이 예제 코드에는 명시적인 종료 처리 로직이 없음
    void Run(uint16_t port) {
        std::string server_address = absl::StrFormat("0.0.0.0:%d", port);

        ServerBuilder builder;
        // 인증 메커니즘 없이 지정된 주소에서 수신 대기
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        // 클라이언트와 통신할 서비스 인스턴스를 등록
        // 여기서는 *비동기* 서비스에 해당함
        builder.RegisterService(&service_);
        // gRPC 런타임과 비동기 통신을 하기 위한 completion queue 획득
        cq_ = builder.AddCompletionQueue();
        // 서버를 최종적으로 생성하고 시작
        server_ = builder.BuildAndStart();
        std::cout << "Server listening on " << server_address << std::endl;

        // 서버의 메인 루프로 진입
        HandleRpcs();
    }

   private:
    // 단일 RPC 요청을 처리하기 위한 상태와 로직을 포함하는 클래스
    class CallData {
       public:
        // 비동기 서버를 나타내는 서비스 인스턴스와
        // gRPC 런타임과 통신하는 completion queue를 전달받음
        CallData(Greeter::AsyncService* service, ServerCompletionQueue* cq)
            : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE) {
            // 생성과 동시에 처리 로직을 시작
            Proceed();
        }

        void Proceed() {
            if (status_ == CREATE) {
                // CREATE 상태에서 PROCESS 상태로 전이
                status_ = PROCESS;

                // CREATE 단계에서는 SayHello 요청 처리를 gRPC 런타임에 등록
                // 여기서 "this"는 요청을 식별하기 위한 태그 역할을 하며,
                // 각 CallData 인스턴스는 서로 다른 요청을 동시에 처리할 수 있음
                service_->RequestSayHello(&ctx_, &request_, &responder_, cq_, cq_, this);
            } else if (status_ == PROCESS) {
                // 현재 요청을 처리하는 동안, 다음 요청을 처리할 새 CallData 생성
                // 이 인스턴스는 FINISH 단계에서 스스로 해제됨
                new CallData(service_, cq_);

                // 실제 요청 처리 로직
                std::string prefix("Hello ");
                reply_.set_message(prefix + request_.name());

                // 처리 완료를 gRPC 런타임에 알림
                // 이 인스턴스의 주소를 태그로 사용
                status_ = FINISH;
                responder_.Finish(reply_, Status::OK, this);
            } else {
                CHECK_EQ(status_, FINISH);
                // FINISH 상태에서는 CallData 인스턴스를 삭제
                delete this;
            }
        }

       private:
        // 비동기 서버에서 gRPC 런타임과 통신하기 위한 서비스 객체
        Greeter::AsyncService* service_;
        // 비동기 이벤트를 전달받는 producer-consumer 큐
        ServerCompletionQueue* cq_;
        // RPC 호출의 컨텍스트
        // 압축, 인증, 메타데이터 송신 등의 설정 가능
        ServerContext ctx_;

        // 클라이언트로부터 수신한 요청
        HelloRequest request_;
        // 클라이언트로 보낼 응답
        HelloReply reply_;

        // 클라이언트로 응답을 보내기 위한 객체
        ServerAsyncResponseWriter<HelloReply> responder_;

        // 간단한 상태 머신 정의
        enum CallStatus { CREATE, PROCESS, FINISH };
        CallStatus status_;  // 현재 상태
    };

    // 필요하다면 여러 스레드에서 실행 가능
    void HandleRpcs() {
        // 최초 요청 처리를 위한 CallData 생성
        new CallData(&service_, cq_.get());
        void* tag;  // 요청을 고유하게 식별하는 태그
        bool ok;
        while (true) {
            // completion queue에서 다음 이벤트를 블로킹 방식으로 대기
            // 이벤트는 tag로 식별되며, 여기서는 CallData 인스턴스 주소
            // Next의 반환값은 cq가 종료 중인지 여부를 알려주므로 반드시 확인해야 함
            CHECK(cq_->Next(&tag, &ok));
            CHECK(ok);
            static_cast<CallData*>(tag)->Proceed();
        }
    }

    std::unique_ptr<ServerCompletionQueue> cq_;
    Greeter::AsyncService service_;
    std::unique_ptr<Server> server_;
};

int main(int argc, char** argv) {
    // absl은 커맨드라인 인자처리 유틸라이브러리
    absl::ParseCommandLine(argc, argv);
    ServerImpl server;
    server.Run(absl::GetFlag(FLAGS_port));

    return 0;
}
