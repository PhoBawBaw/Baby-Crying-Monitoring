#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include <chrono>
#include <thread>
#include <filesystem>

#include <grpcpp/grpcpp.h>
#include <opencv2/opencv.hpp>

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
namespace fs = std::filesystem;

class GreeterClient {
 public:
  GreeterClient(std::shared_ptr<Channel> channel)
      : stub_(Greeter::NewStub(channel)) {}

  void StreamVideo(const std::string& user) {
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
        StreamVideoFile(user, current_file);
      }

      std::this_thread::sleep_for(std::chrono::seconds(1));  // Check for new files every second
    }
  }

 private:
  std::unique_ptr<Greeter::Stub> stub_;

  // Function to stream a single video file
  void StreamVideoFile(const std::string& user, const std::string& video_file) {
    ClientContext context;
    std::shared_ptr<ClientReaderWriter<HelloRequest, HelloReply>> stream(
        stub_->StreamVideo(&context));

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

      HelloRequest request;
      request.set_name(user);
      request.set_video_frame(std::string(buf.begin(), buf.end()));

      if (!stream->Write(request)) {
        break;  // Exit if the server closes the stream
      }

      HelloReply reply;
      if (stream->Read(&reply)) {
        std::cout << "Server response: " << reply.message() << std::endl;
      }

      // Check for a new file to switch to while streaming
      std::string latest_file = GetLatestFile("/home/jihyoung/record");
      if (!latest_file.empty() && latest_file != video_file) {
        std::cout << "Newer file detected. Switching to: " << latest_file << std::endl;
        cap.release();  // Release the current file
        stream->WritesDone();  // End the current stream properly
        return;  // Exit the function to switch to the new file
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(frame_duration_ms));
    }

    stream->WritesDone();
    Status status = stream->Finish();
    if (!status.ok()) {
      std::cout << "StreamVideo RPC failed: " << status.error_message() << std::endl;
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
  greeter.StreamVideo(user);
  return 0;
}

