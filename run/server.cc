#include <iostream>
#include <thread>
#include <cstdlib>
#include <filesystem>
#include <vector>
#include <atomic>
#include <chrono>

// 종료 플래그
std::atomic<bool> ffmpeg_running(true);
std::atomic<bool> predict_running(true);

// ffmpeg 실행 함수
void run_ffmpeg() {
    std::system("ffmpeg -i rtsp://<ip address>:<port>/test "
                "-vn -acodec pcm_s16le -ar 44100 -ac 2 -f segment -segment_time 5 -reset_timestamps 1 "
                "-strftime 1 \"/baby/audio/audio_%Y-%m-%d_%H-%M-%S.wav\" "
                "-c:v copy -c:a copy "
                "-hls_time 1 "
                "-hls_playlist_type event "
                "-fflags nobuffer+genpts "
                "-tune zerolatency -preset ultrafast "
		"-use_wallclock_as_timestamps 1 "
                "-muxdelay 0 "
                "-f hls /baby/stream/stream.m3u8 "
                "-f segment -segment_time 60 -reset_timestamps 1 "
                "-strftime 1 \"/baby/record/record_%Y-%m-%d_%H-%M-%S.mp4\"");
    
    ffmpeg_running.store(false); // ffmpeg가 종료됨을 표시
}

// predict.py 실행 함수
void run_predict() {
    std::system("python3 /baby/predict.py");
    
    predict_running.store(false); // predict.py가 종료됨을 표시
}

int main() {
    // 디렉토리 초기화 작업
    std::vector<std::string> dirs = {"/baby/stream", "/baby/audio", "/baby/record"};
    for (const auto& dir : dirs) {
        if (!std::filesystem::exists(dir)) {
            std::filesystem::create_directories(dir);
        } else {
            std::filesystem::remove_all(dir);
            std::filesystem::create_directories(dir);
        }
    }

    // 스레드 생성
    std::thread ffmpeg_thread(run_ffmpeg);
    std::thread predict_thread(run_predict);

    // 종료 신호 감지 및 프로그램 종료
    while (ffmpeg_running.load() && predict_running.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(10)); // 10초
    }

    if (!ffmpeg_running.load()) {
        std::cout << "ffmpeg thread is terminating. Forcing predict.py to exit." << std::endl;
        std::system("pkill -f predict.py"); // predict.py 프로세스 강제 종료
    } else {
        std::cout << "predict thread is terminating. Forcing ffmpeg to exit." << std::endl;
        std::system("pkill -f ffmpeg"); // ffmpeg 프로세스 강제 종료
    }

    // 모든 프로세스가 종료되면 프로그램 종료
    std::cout << "Program terminated as all processes have exited." << std::endl;

    // 스레드 조인
    ffmpeg_thread.join();
    predict_thread.join();

    return 0;
}
