from flask import Flask, send_from_directory

app = Flask(__name__)

# HLS 파일을 제공할 경로 (FFmpeg로 생성된 HLS 파일 경로)
HLS_OUTPUT_PATH = '/home/zzanggu/GStreamer/stream/'

# 메인 페이지
@app.route('/')
def index():
    return app.send_static_file('index.html')

# HLS 스트림 제공
@app.route('/stream.m3u8')
def stream_m3u8():
    return send_from_directory(HLS_OUTPUT_PATH, 'stream.m3u8')

# TS 세그먼트 제공
@app.route('/<path:filename>')
def stream_ts(filename):
    return send_from_directory(HLS_OUTPUT_PATH, filename)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)