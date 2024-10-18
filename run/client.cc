#include <iostream>
#include <thread>
#include <cstdlib>  // For system()
#include <sys/stat.h> // For mkdir
#include <unistd.h>  // For access

// 함수: 디렉토리 생성
void create_bin_directory() {
    const char* home = getenv("HOME");
    std::string path = std::string(home) + "/Baby-Crying-Monitoring/bin";

    // 디렉토리가 없으면 생성
    if (mkdir(path.c_str(), 0755) == -1) {
        if (errno != EEXIST) {
            std::cerr << "Failed to create directory: " << path << std::endl;
            exit(1);
        }
    }
}

// 함수: make 실행
void run_make() {
    int result = system("cd ~/Baby-Crying-Monitoring && make clean && make");
    if (result == 0) {
        std::cout << "Make executed successfully" << std::endl;
    } else {
        std::cerr << "Error occurred while running make" << std::endl;
    }
}

// 함수: RTSP 실행
void run_rtsp() {
    int result = system("~/Baby-Crying-Monitoring/bin/rtsp");
    if (result == 0) {
        std::cout << "RTSP executed successfully" << std::endl;
    } else {
        std::cerr << "Error occurred while running RTSP" << std::endl;
    }
}

// 함수: dht22_reader 실행
void run_dht22_reader() {
    int result = system("~/Baby-Crying-Monitoring/bin/dht22_reader");
    if (result == 0) {
        std::cout << "dht22_reader executed successfully" << std::endl;
    } else {
        std::cerr << "Error occurred while running dht22_reader" << std::endl;
    }
}

int main() {
    // bin 디렉토리 생성
    create_bin_directory();

    // make 실행
    run_make();

    // 각각의 스레드에서 RTSP와 dht22_reader 실행
    std::thread rtsp_thread(run_rtsp);
    std::thread dht22_reader_thread(run_dht22_reader);

    // 두 스레드가 완료될 때까지 기다림
    rtsp_thread.join();
    dht22_reader_thread.join();

    std::cout << "All tasks completed" << std::endl;

    return 0;
}
