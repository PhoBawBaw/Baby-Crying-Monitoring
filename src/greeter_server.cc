#include <iostream>
#include <fstream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#ifdef BAZEL_BUILD
#include "examples/protos/helloworld.grpc.pb.h"
#else
#include "helloworld.grpc.pb.h"
#endif

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::ServerReaderWriter;
using helloworld::Greeter;
using helloworld::HelloReply;
using helloworld::HelloRequest;

class GreeterServiceImpl final : public Greeter::Service {
 public:
  // 양방향 스트리밍을 처리하는 메서드로 변경
  Status StreamVideo(ServerContext* context,
                     ServerReaderWriter<HelloReply, HelloRequest>* stream) override {
    HelloRequest request;
    int frame_count = 0;

    std::ofstream file("received_video_stream.mp4", std::ios::binary); // 수신된 비디오 데이터를 저장할 파일

    while (stream->Read(&request)) {  // 클라이언트로부터 비디오 프레임을 읽음
      if (!request.video_frame().empty()) {
        file.write(request.video_frame().data(), request.video_frame().size());
        frame_count++;
        std::cout << "Received frame " << frame_count
                  << " of size: " << request.video_frame().size() << " bytes." << std::endl;
      }

      HelloReply reply;
      reply.set_message("Frame " + std::to_string(frame_count) + " received");
      stream->Write(reply);  // 각 프레임 수신 후 클라이언트에 응답
    }

    file.close();
    std::cout << "Finished receiving video stream." << std::endl;

    return Status::OK;
  }
};

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  GreeterServiceImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);

  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  server->Wait();
}

int main(int argc, char** argv) {
  RunServer();

  return 0;
}

