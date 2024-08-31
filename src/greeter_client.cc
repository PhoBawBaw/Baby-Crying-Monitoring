#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include <chrono>
#include <thread>

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

    // 비디오 파일에서 프레임을 캡처합니다.
    cv::VideoCapture cap("video.mp4");  // 비디오 파일 경로를 사용합니다.
    if (!cap.isOpened()) {
      std::cerr << "Failed to open video file" << std::endl;
      return;
    }

    cv::Mat frame;
    const int fps = 30;  // 원하는 FPS 설정
    const int frame_duration_ms = 1000 / fps;  // 각 프레임 간의 지연 시간 (밀리초)

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

      // 30fps에 맞추기 위해 각 프레임 사이에 33ms 대기
      std::this_thread::sleep_for(std::chrono::milliseconds(frame_duration_ms));
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

