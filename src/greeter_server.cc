#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include <opencv2/opencv.hpp>  // OpenCV 헤더 추가

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

    while (stream->Read(&request)) {  // 클라이언트로부터 비디오 프레임을 읽음
      if (!request.video_frame().empty()) {
        std::vector<uchar> buf(request.video_frame().begin(), request.video_frame().end());
        cv::Mat frame = cv::imdecode(buf, cv::IMREAD_COLOR);  // JPEG로 인코딩된 프레임을 디코딩합니다.

        if (!frame.empty()) {
          cv::imshow("Received Video Stream", frame);  // 수신된 프레임을 화면에 표시합니다.
          cv::waitKey(1);  // 키 입력 대기 없이 다음 프레임으로 넘어갑니다.
          frame_count++;
          std::cout << "Received frame " << frame_count
                    << " of size: " << request.video_frame().size() << " bytes." << std::endl;
        }
      }

      HelloReply reply;
      reply.set_message("Frame " + std::to_string(frame_count) + " received");
      stream->Write(reply);  // 각 프레임 수신 후 클라이언트에 응답
    }

    cv::destroyAllWindows();  // 모든 OpenCV 윈도우를 닫습니다.
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

