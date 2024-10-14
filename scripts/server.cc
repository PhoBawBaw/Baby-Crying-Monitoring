#include <iostream>
#include <thread>
#include <cstdlib>
#include <filesystem>
#include <vector>

// ffmpeg 실행 함수
void run_ffmpeg() {
    std::system("ffmpeg -i rtsp://<ip address>:<port>/test "
                "-vn -acodec pcm_s16le -ar 44100 -ac 2 -f segment -segment_time 5 -reset_timestamps 1 "
                "-strftime 1 \"/app/audio/audio_%Y-%m-%d_%H-%M-%S.wav\" "
                "-c:v copy -c:a copy "
                "-hls_time 1 -hls_list_size 1 "
                "-hls_flags delete_segments "
                "-hls_playlist_type event "
                "-fflags nobuffer+genpts "
                "-use_wallclock_as_timestamps 1 "
                "-muxdelay 0 "
                "-f hls /app/stream/stream.m3u8 "
                "-f segment -segment_time 60 -reset_timestamps 1 "
                "-strftime 1 \"/app/record/record_%Y-%m-%d_%H-%M-%S.mp4\"");
}

// predict.py 실행 함수
void run_predict() {
    std::system("python /app/predict.py");
}

int main() {
    // 디렉토리 초기화 작업
    std::vector<std::string> dirs = {"/app/stream", "/app/audio", "/app/record"};
    for (const auto& dir : dirs) {
        if (!std::filesystem::exists(dir)) {
            std::filesystem::create_directories(dir);
        } else {
            std::filesystem::remove_all(dir); // 기존 파일 삭제
            std::filesystem::create_directories(dir); // 다시 생성
        }
    }

    std::thread ffmpeg_thread(run_ffmpeg);
    std::thread predict_thread(run_predict);
    
    ffmpeg_thread.join();
    predict_thread.join();

    std::cout << "Program terminated." << std::endl;
    return 0;
}
