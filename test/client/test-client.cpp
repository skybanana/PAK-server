#include "test-client.h"

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/log/check.h>
#include <absl/log/initialize.h>

ABSL_FLAG(std::string, target, "localhost:50051", "Server address");

using echo::EchoReply;
using echo::EchoRequest;
using echo::EchoService;
using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;

TestClient::TestClient(std::shared_ptr<Channel> channel)
    : stub_(EchoService::NewStub(std::move(channel))) {};

std::string TestClient::echo(const std::string& message) {
    EchoRequest request;
    request.set_message(message);

    EchoReply reply;
    ClientContext context;
    CompletionQueue cq;
    Status status;

    std::unique_ptr<ClientAsyncResponseReader<EchoReply>> rpc(
        stub_->AsyncEcho(&context, request, &cq));

    rpc->Finish(&reply, &status, (void*)1);
    void* got_tag;
    bool ok = false;
    CHECK(cq.Next(&got_tag, &ok));
    CHECK_EQ(got_tag, (void*)1);
    CHECK(ok);

    if (status.ok()) {
        return reply.message();
    }
    return "RPC failed";
}

int main(int argc, char** argv) {
    absl::ParseCommandLine(argc, argv);
    absl::InitializeLog();
    std::string target_str = absl::GetFlag(FLAGS_target);

    TestClient client(grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
    std::string message;
    while (true) {
        std::cin >> message;
        std::string reply = client.echo(message);
        std::cout << "Server received: " << reply << "\n";
    }
    return 0;
};