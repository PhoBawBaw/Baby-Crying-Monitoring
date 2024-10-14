#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <iostream>
#include <string>
#include <sstream>
#include <cstdio>
#include <regex>

std::string get_microphone_device() {
    // arecord -l 명령 실행
    FILE *pipe = popen("arecord -l", "r");
    if (!pipe) return "";

    char buffer[128];
    std::string result = "";

    // 명령 출력 읽기
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);

    // 정규식을 사용해 card와 device 값을 추출
    std::regex regex("card (\\d+):.*device (\\d+):");
    std::smatch match;
    if (std::regex_search(result, match, regex) && match.size() > 2) {
        std::string card = match.str(1);   // card 값
        std::string device = match.str(2); // device 값
        return "hw:" + card + "," + device;
    }

    return ""; // 값이 없으면 빈 문자열 반환
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    // 마이크 디바이스 번호 가져오기
    std::string microphone_device = get_microphone_device();
    
    // 번호를 찾았을 때와 찾지 못했을 때의 처리
    if (microphone_device.empty()) {
        std::cerr << "Failed to detect microphone device!" << std::endl;
        return -1;
    } else {
        std::cout << "Detected microphone device: " << microphone_device << std::endl;
    }

    // RTSP 서버 생성
    GstRTSPServer *server = gst_rtsp_server_new();

    // 서버 주소와 포트 설정
    gst_rtsp_server_set_address(server, "0.0.0.0");  // 모든 인터페이스에서 연결 허용
    gst_rtsp_server_set_service(server, "8554");     // 포트 8554에서 RTSP 서비스

    GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);

    // 비디오와 오디오 스트림 파이프라인 (동적으로 마이크 디바이스 설정)
    std::string launch_pipeline =
    "( rtpbin name=rtpbin "
    "libcamerasrc ! video/x-raw,width=640,height=480,framerate=15/1,format=I420 ! videoconvert ! "
    "x264enc tune=zerolatency bitrate=500 speed-preset=superfast ! rtph264pay name=pay0 pt=96 "
    "alsasrc device=" + microphone_device + " ! audio/x-raw,format=S16LE,rate=44100,channels=1 ! audioconvert ! audioresample ! "
    "voaacenc ! rtpmp4gpay name=pay1 pt=97 )";

    // RTSP 미디어 팩토리 생성
    GstRTSPMediaFactory *factory = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(factory, launch_pipeline.c_str());

    // 스트림을 /test 경로에 마운트
    gst_rtsp_mount_points_add_factory(mounts, "/test", factory);
    g_object_unref(mounts);

    // RTSP 서버 시작
    gst_rtsp_server_attach(server, NULL);

    g_print("RTSP Stream ready at rtsp://0.0.0.0:8554/test\n");

    // 메인 루프 실행
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    return 0;
}
