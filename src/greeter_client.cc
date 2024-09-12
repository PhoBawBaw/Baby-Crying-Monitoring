#include <iostream>
#include <memory>   // std::unique_ptr 사용을 위한 헤더 추가
#include <string>   // std::string 사용을 위한 헤더 추가
#include <fstream>
#include <chrono>
#include <thread>
#include <filesystem>
#include <grpcpp/grpcpp.h>
#include <opencv2/opencv.hpp>
#include <future>

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
using helloworld::VideoRequest;
using helloworld::AudioRequest;
namespace fs = std::filesystem;

class GreeterClient {
 public:
  GreeterClient(std::shared_ptr<Channel> channel)
      : video_stub_(Greeter::NewStub(channel)),
        audio_stub_(Greeter::NewStub(channel)) {}

  void StreamVideoAndAudio(const std::string& user) {
    std::string video_directory = "/home/jihyoung/record";
    std::string current_file;

    while (true) {
      std::string latest_file = GetLatestFile(video_directory);
      if (latest_file.empty()) {
        std::cerr << "No video files found" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));  // Wait and retry
        continue;
      }

      if (latest_file != current_file) {
        current_file = latest_file;
        std::cout << "Streaming new file: " << current_file << std::endl;

        // 비디오와 오디오를 병렬로 스트리밍
        auto video_stream = std::async(std::launch::async, &GreeterClient::StreamVideoFile, this, user, current_file);
        auto audio_stream = std::async(std::launch::async, &GreeterClient::StreamAudioFile, this, user, current_file);

        // 두 작업이 모두 끝날 때까지 대기
        video_stream.get();
        audio_stream.get();
      }

      std::this_thread::sleep_for(std::chrono::seconds(1));  // Check for new files every second
    }
  }

 private:
  std::unique_ptr<Greeter::Stub> video_stub_;
  std::unique_ptr<Greeter::Stub> audio_stub_;

  // Function to stream a single video file
  void StreamVideoFile(const std::string& user, const std::string& video_file) {
    ClientContext context;
    std::shared_ptr<ClientReaderWriter<VideoRequest, HelloReply>> stream(
        video_stub_->StreamVideo(&context));

    cv::VideoCapture cap(video_file);  // Open the specified video file
    if (!cap.isOpened()) {
      std::cerr << "Failed to open video file: " << video_file << std::endl;
      return;
    }

    cv::Mat frame;
    const int fps = 30;
    const int frame_duration_ms = 1000 / fps;

    while (cap.read(frame)) {
      std::vector<uchar> buf;
      cv::imencode(".jpg", frame, buf);

      VideoRequest request;
      request.set_name(user);
      request.set_video_frame(std::string(buf.begin(), buf.end()));

      if (!stream->Write(request)) {
        break;  // Exit if the server closes the stream
      }

      HelloReply reply;
      if (stream->Read(&reply)) {
        std::cout << "Video Server response: " << reply.message() << std::endl;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(frame_duration_ms));
    }

    stream->WritesDone();
    Status status = stream->Finish();
    if (!status.ok()) {
      std::cout << "StreamVideo RPC failed: " << status.error_message() << std::endl;
    }
  }

  void StreamAudioFile(const std::string& user, const std::string& video_file) {
      ClientContext context;
      std::shared_ptr<ClientReaderWriter<AudioRequest, HelloReply>> stream(
          audio_stub_->StreamAudio(&context));

      std::string audio_file = video_file.substr(0, video_file.find_last_of('.')) + ".aac";
      std::ifstream audio_stream(audio_file, std::ios::binary);
      if (!audio_stream.is_open()) {
        std::cerr << "Failed to open audio file: " << audio_file << std::endl;
        return;
      }

      const int audio_frame_size = 512;  // 오디오 데이터를 읽어올 버퍼 크기
      std::vector<char> audio_buffer(audio_frame_size);

      // 비디오와 동일한 속도로 오디오 프레임을 전송하도록 대기 시간 추가
      const int fps = 30;
      const int frame_duration_ms = 1000 / fps;

      while (audio_stream.read(audio_buffer.data(), audio_buffer.size())) {
        AudioRequest request;
        request.set_name(user);
        request.set_audio_frame(std::string(audio_buffer.data(), audio_stream.gcount()));

        if (!stream->Write(request)) {
          break;  // Exit if the server closes the stream
        }

        HelloReply reply;
        if (stream->Read(&reply)) {
          std::cout << "Audio Server response: " << reply.message() << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(frame_duration_ms));  // 비디오와 동일한 주기로 오디오 전송
      }

      stream->WritesDone();
      Status status = stream->Finish();
      if (!status.ok()) {
        std::cout << "StreamAudio RPC failed: " << status.error_message() << std::endl;
      }
  }

  // Function to get the most recent file from a directory
  std::string GetLatestFile(const std::string& directory_path) {
    std::string latest_file;
    std::time_t latest_time = 0;

    for (const auto& entry : fs::directory_iterator(directory_path)) {
      if (entry.is_regular_file()) {
        std::string filename = entry.path().filename().string();
        if (filename == "onair.mp4") {
          continue;  // Skip the file named "onair.mp4"
        }

        auto ftime = fs::last_write_time(entry);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
        );
        std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);
        if (cftime > latest_time) {
          latest_time = cftime;
          latest_file = entry.path().string();
        }
      }
    }

    return latest_file;
  }
};

int main(int argc, char** argv) {
  std::string target_str("localhost:50051");
  GreeterClient greeter(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
  std::string user("world");
  greeter.StreamVideoAndAudio(user);
  return 0;
}

