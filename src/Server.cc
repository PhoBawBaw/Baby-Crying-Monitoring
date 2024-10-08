#include <iostream>
#include <cstdlib>
#include <sys/stat.h>

// RTSP 스트림에서 음성을 추출하고 동시에 HLS 스트림으로 변환
void process_audio_and_hls() {
    std::string ffmpeg_cmd = "ffmpeg -i rtsp://<rtsp server ip address>:18555/test "
                             "-vn -acodec pcm_s16le -ar 44100 -ac 2 -f segment -segment_time 60 -reset_timestamps 1 "
                             "-strftime 1 \"/home/zzanggu/rtsp/audio/%Y-%m-%d_%H-%M-%S.wav\" "
                             "-c:v copy -c:a copy "
                             "-hls_time 1 -hls_list_size 1 "
                             "-hls_flags delete_segments "
                             "-hls_playlist_type event "
                             "-fflags nobuffer+genpts "
                             "-use_wallclock_as_timestamps 1 "
                             "-muxdelay 0 "
                             "-f hls /home/zzanggu/rtsp/stream/stream.m3u8";
    
    std::system(ffmpeg_cmd.c_str());
}

int main() {
    //rtsp 하위 stream과 audio 디렉 없으면 생성

    // 작업 실행
    process_audio_and_hls();

    std::cout << "Audio and HLS stream created." << std::endl;

    return 0;
}
