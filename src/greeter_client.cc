#include <iostream>
#include <memory>
#include <string>
#include <fstream>  // 파일 읽기를 위한 헤더 추가

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"

#include <grpcpp/grpcpp.h>

#ifdef BAZEL_BUILD
#include "examples/protos/helloworld.grpc.pb.h"
#else
#include "helloworld.grpc.pb.h"
#endif

ABSL_FLAG(std::string, target, "localhost:50051", "Server address");
ABSL_FLAG(std::string, video_file, "", "Path to the video file");  // 비디오 파일 경로 플래그 추가

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using helloworld::Greeter;
using helloworld::HelloReply;
using helloworld::HelloRequest;

class GreeterClient {
 public:
  GreeterClient(std::shared_ptr<Channel> channel)
      : stub_(Greeter::NewStub(channel)) {}

  std::string SayHello(const std::string& user, const std::string& video_file) {
    HelloRequest request;
    request.set_name(user);

    // 비디오 파일을 읽어와서 요청에 추가합니다.
    std::ifstream file(video_file, std::ios::binary);
    if (file) {
      std::string video_data((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
      request.set_video(video_data);
    } else {
      std::cerr << "Failed to open video file: " << video_file << std::endl;
      return "Failed to open video file";
    }

    HelloReply reply;
    ClientContext context;

    Status status = stub_->SayHello(&context, request, &reply);

    if (status.ok()) {
      return reply.message();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

 private:
  std::unique_ptr<Greeter::Stub> stub_;
};

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);
  std::string target_str = absl::GetFlag(FLAGS_target);
  std::string video_file = absl::GetFlag(FLAGS_video_file);  // 비디오 파일 플래그 가져오기

  GreeterClient greeter(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
  std::string user("world");
  std::string reply = greeter.SayHello(user, video_file);
  std::cout << "Greeter received: " << reply << std::endl;

  return 0;
}
