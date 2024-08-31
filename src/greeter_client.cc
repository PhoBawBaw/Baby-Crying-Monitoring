#include <iostream>
#include <memory>
#include <string>
#include <wiringPi.h>
#include <fstream>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"

#include <grpcpp/grpcpp.h>

#ifdef BAZEL_BUILD
#include "examples/protos/helloworld.grpc.pb.h"
#else
#include "helloworld.grpc.pb.h"
#endif

using namespace std;

ABSL_FLAG(string, target, "183.104.150.59:50051", "Server address");

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using helloworld::Greeter;
using helloworld::HelloReply;
using helloworld::HelloRequest;

#define MAX_TIMINGS 85 // 시작 신호 + 센서 응답 + 5바이트 데이터
#define DHT_PIN 17 // GPIO 17번 핀 사용

int sensor_data[5] = {0, 0, 0, 0, 0}; // 5바이트 데이터(온습도 + 체크섬)

class GreeterClient {
public:
    GreeterClient(shared_ptr<Channel> channel)
        : stub_(Greeter::NewStub(channel)) {}

    string SayHello(const string& user, float temperature, float humidity) {
        HelloRequest request;
        request.set_name(user);
        request.set_temperature(temperature);
        request.set_humidity(humidity);

        HelloReply reply;

        ClientContext context;

        Status status = stub_->SayHello(&context, request, &reply);

        if (status.ok()) {
            return reply.message();
        } else {
            cout << status.error_code() << ": " << status.error_message() << endl;
            return "RPC failed";
        }
    }

private:
    unique_ptr<Greeter::Stub> stub_;
};

void read_dht_data(float &temperature, float &humidity) {
    uint8_t laststate = HIGH;
    uint8_t counter = 0;
    uint8_t j = 0, i;

    //이전 온습도 데이터 초기화
    memset(sensor_data, 0, sizeof(sensor_data));


    pinMode(DHT_PIN, OUTPUT);
    digitalWrite(DHT_PIN, LOW);
    delay(18);
    digitalWrite(DHT_PIN, HIGH);
    delayMicroseconds(40);
    pinMode(DHT_PIN, INPUT);

    for (i = 0; i < MAX_TIMINGS; i++) {
        counter = 0;
        while (digitalRead(DHT_PIN) == laststate) {
            counter++;
            delayMicroseconds(1);
            if (counter == 255) {
                break;
            }
        }
        laststate = digitalRead(DHT_PIN);

        if (counter == 255) break;

        if ((i >= 4) && (i % 2 == 0)) {
            sensor_data[j / 8] <<= 1;
            if (counter > 16)
                sensor_data[j / 8] |= 1;
            j++;
        }
    }

    if ((j >= 40) && (sensor_data[4] == ((sensor_data[0] + sensor_data[1] + sensor_data[2] + sensor_data[3]) & 0xFF))) {
        humidity = (float)((sensor_data[0] << 8) + sensor_data[1]) / 10.0;  // DHT22는 16비트 습도 데이터
        temperature = (float)(((sensor_data[2] & 0x7F) << 8) + sensor_data[3]) / 10.0;  // DHT22는 16비트 온도 데이터

        if (sensor_data[2] & 0x80) temperature = -temperature;  // 음수 온도 처리
        cout << "temperature: " << temperature << "/ humidity:" << humidity << endl;
    } else {
        cout << "Data not good, skip" << endl;
    }
}

int main(int argc, char** argv) {
    absl::ParseCommandLine(argc, argv);

    if (wiringPiSetupGpio() == -1)
        return 1;

    string target_str = absl::GetFlag(FLAGS_target);
    GreeterClient greeter(grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

    while (true) {
        float temperature = 0.0, humidity = 0.0;
        read_dht_data(temperature, humidity);
        string user("world");
        string reply = greeter.SayHello(user, temperature, humidity);
        cout << "Greeter received: " << reply << endl;
        delay(2000); // 2초마다 읽기
    }

    return 0;
}
