#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include <grpcpp/grpcpp.h>
#include <opencv2/opencv.hpp>  // OpenCV 라이브러리를 사용하여 비디오 프레임을 캡처합니다.

#ifdef BAZEL_BUILD
#include "examples/protos/helloworld.grpc.pb.h"
#else
#include "helloworld.grpc.pb.h"
#endif

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ClientReaderWriter;
using helloworld::Greeter;
using helloworld::HelloReply;
using helloworld::HelloRequest;

class GreeterClient {
 public:
  GreeterClient(std::shared_ptr<Channel> channel)
      : stub_(Greeter::NewStub(channel)) {}

  void StreamVideo(const std::string& user) {
    ClientContext context;
    std::shared_ptr<ClientReaderWriter<HelloRequest, HelloReply>> stream(
        stub_->StreamVideo(&context));

    // 라즈베리파이 카메라에서 비디오 스트림을 캡처합니다.
    cv::VideoCapture cap(0);  // 0은 기본 카메라 장치를 의미합니다.
    if (!cap.isOpened()) {
      std::cerr << "Failed to open camera" << std::endl;
      return;
    }

    cv::Mat frame;
    while (cap.read(frame)) {
      std::vector<uchar> buf;
      cv::imencode(".jpg", frame, buf);  // 프레임을 JPEG로 인코딩합니다.

      HelloRequest request;
      request.set_name(user);
      request.set_video_frame(std::string(buf.begin(), buf.end()));

      // 프레임을 서버로 전송합니다.
      if (!stream->Write(request)) {
        break;  // 서버가 스트림을 닫은 경우 반복을 종료합니다.
      }

      HelloReply reply;
      if (stream->Read(&reply)) {
        std::cout << "Server response: " << reply.message() << std::endl;
      }
    }

    stream->WritesDone();
    Status status = stream->Finish();
    if (!status.ok()) {
      std::cout << "StreamVideo RPC failed: " << status.error_message() << std::endl;
    }
  }

 private:
  std::unique_ptr<Greeter::Stub> stub_;
};

int main(int argc, char** argv) {
  std::string target_str("localhost:50051");
  GreeterClient greeter(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
  std::string user("world");
  greeter.StreamVideo(user);
  return 0;
}
