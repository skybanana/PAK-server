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
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <iostream>
#include <memory>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/initialize.h"
#include "absl/strings/str_format.h"

#ifdef BAZEL_BUILD
#include "examples/protos/helloworld.grpc.pb.h"
#else
#include "helloworld.grpc.pb.h"
#endif

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using helloworld::Greeter;
using helloworld::HelloReply;
using helloworld::HelloRequest;

// 서비스가 사용할 서버 포트 (기본값: 50051)
ABSL_FLAG(uint16_t, port, 50051, "Server port for the service");

// 서버의 동작(비즈니스 로직)과 데이터를 담당하는 구현체
class GreeterServiceImpl final : public Greeter::Service {
    Status SayHello(ServerContext* context,
                    const HelloRequest* request,
                    HelloReply* reply) override {
        std::string prefix("Hello ");
        reply->set_message(prefix + request->name());
        return Status::OK;
    }
};

void RunServer(uint16_t port) {
    std::string server_address = absl::StrFormat("0.0.0.0:%d", port);
    GreeterServiceImpl service;

    // 기본 헬스 체크 서비스 활성화
    grpc::EnableDefaultHealthCheckService(true);
    // proto reflection 플러그인 초기화 (서버가 자신의 proto 정보를 노출)
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    ServerBuilder builder;
    // 인증 메커니즘 없이 지정된 주소에서 수신 대기
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // 클라이언트와 통신할 서비스 인스턴스를 등록
    // 이 경우 *동기(synchronous)* 서비스에 해당함
    builder.RegisterService(&service);
    // 서버를 최종적으로 생성하고 시작
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    // 서버가 종료될 때까지 대기
    // 이 함수가 반환되려면, 다른 스레드에서 서버 종료를 호출해야 함
    server->Wait();
}

int main(int argc, char** argv) {
    absl::ParseCommandLine(argc, argv);
    absl::InitializeLog();
    RunServer(absl::GetFlag(FLAGS_port));
    return 0;
}
