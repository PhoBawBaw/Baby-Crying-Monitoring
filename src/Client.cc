#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <string>

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    // RTSP 서버 생성
    GstRTSPServer *server = gst_rtsp_server_new();

    // 서버 주소와 포트 설정
    gst_rtsp_server_set_address(server, "0.0.0.0");  // 모든 인터페이스에서 연결 허용
    gst_rtsp_server_set_service(server, "8555");     // 포트 8555에서 RTSP 서비스

    GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);

    // 비디오와 오디오 스트림 파이프라인
    std::string launch_pipeline =
    "( rtpbin name=rtpbin "
    "libcamerasrc ! video/x-raw,width=640,height=480,framerate=15/1,format=I420 ! videoconvert ! "
    "x264enc tune=zerolatency bitrate=500 speed-preset=superfast ! rtph264pay name=pay0 pt=96 "
    "alsasrc device=hw:3,0 ! audio/x-raw,format=S16LE,rate=44100,channels=1 ! audioconvert ! audioresample ! "
    "voaacenc ! rtpmp4gpay name=pay1 pt=97 )";

    // RTSP 미디어 팩토리 생성
    GstRTSPMediaFactory *factory = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(factory, launch_pipeline.c_str());

    // 스트림을 /test 경로에 마운트
    gst_rtsp_mount_points_add_factory(mounts, "/test", factory);
    g_object_unref(mounts);

    // RTSP 서버 시작
    gst_rtsp_server_attach(server, NULL);

    g_print("RTSP Stream ready at rtsp://0.0.0.0:8555/test\n");

    // 메인 루프 실행
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    return 0;
}
