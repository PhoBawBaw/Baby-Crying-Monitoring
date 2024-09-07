#include <iostream>
#include <vector>
#include <fstream>
#include <grpcpp/grpcpp.h>
#include <opencv2/opencv.hpp>
#include <httplib.h>  // cpp-httplib 헤더 추가

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
    Status StreamVideo(ServerContext* context,
                       ServerReaderWriter<HelloReply, HelloRequest>* stream) override {
        HelloRequest request;
        int frame_count = 0;

        // HTTP 클라이언트를 설정합니다.
        httplib::Client cli("http://localhost:5000");  // Flask 서버 주소

        while (stream->Read(&request)) {  // 클라이언트로부터 비디오와 오디오 프레임을 읽음
            // 비디오 데이터 처리 및 전송
            if (!request.video_frame().empty()) {
                std::vector<uchar> buf(request.video_frame().begin(), request.video_frame().end());
                cv::Mat frame = cv::imdecode(buf, cv::IMREAD_COLOR);  // JPEG로 인코딩된 프레임을 디코딩합니다.

                if (!frame.empty()) {
                    // JPEG로 인코딩하여 Flask 서버로 전송합니다.
                    std::vector<uchar> encoded_frame;
                    cv::imencode(".jpg", frame, encoded_frame);
                    std::string frame_data(encoded_frame.begin(), encoded_frame.end());

                    // HTTP POST 요청을 보내어 비디오 프레임을 Flask 서버로 전송합니다.
                    httplib::Headers headers = {{"Content-Type", "image/jpeg"}};
                    auto res = cli.Post("/upload_frame", headers, frame_data, "image/jpeg");

                    if (res.error() != httplib::Error::Success) {
                        std::cerr << "Failed to send frame to Flask server" << std::endl;
                    }

                    frame_count++;
                    std::cout << "Received and sent frame " << frame_count << " of size: " << request.video_frame().size() << " bytes." << std::endl;
                }
            }

            // 오디오 데이터 처리 및 전송
            if (!request.audio_frame().empty()) {
                std::string audio_data = request.audio_frame();

                // HTTP POST 요청을 보내어 오디오 프레임을 Flask 서버로 전송합니다.
                httplib::Headers headers = {{"Content-Type", "audio/aac"}};
                auto res = cli.Post("/upload_audio", headers, audio_data, "audio/aac");

                if (res.error() != httplib::Error::Success) {
                    std::cerr << "Failed to send audio to Flask server" << std::endl;
                }

                std::cout << "Received and sent audio frame of size: " << request.audio_frame().size() << " bytes." << std::endl;
            }

            HelloReply reply;
            reply.set_message("Frame " + std::to_string(frame_count) + " received and sent");
            stream->Write(reply);  // 각 프레임 수신 후 클라이언트에 응답
        }

        std::cout << "Finished receiving and sending video and audio stream." << std::endl;
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

