from flask import Flask, render_template, request, Response, stream_with_context
import cv2
import numpy as np
import io

app = Flask(__name__)

frame_buffer = None  # 글로벌 변수로 프레임을 저장
audio_buffer = io.BytesIO()  # 오디오 버퍼를 메모리에 저장

def generate_frames():
    global frame_buffer
    while True:
        if frame_buffer is not None:
            # 현재 프레임을 가져와서 JPEG로 인코딩
            ret, buffer = cv2.imencode('.jpg', frame_buffer)
            if not ret:
                continue
            frame = buffer.tobytes()
            yield (b'--frame\r\n'
                   b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')
        else:
            # 빈 프레임 또는 대기
            yield (b'--frame\r\n'
                   b'Content-Type: image/jpeg\r\n\r\n' + b'\r\n')

def generate_audio():
    while True:
        if audio_buffer.tell() > 0:
            audio_buffer.seek(0)
            data = audio_buffer.read()
            audio_buffer.seek(0)  # Reset buffer for future writes
            yield (b'--audio\r\n'
                   b'Content-Type: audio/aac\r\n\r\n' + data + b'\r\n')
        else:
            yield (b'--audio\r\n'
                   b'Content-Type: audio/aac\r\n\r\n' + b'\r\n')

@app.route('/')
def index():
    return render_template('index.html')  # templates 폴더에서 index.html 파일을 렌더링

@app.route('/video_feed')
def video_feed():
    # 비디오 스트림을 위한 코드
    return Response(generate_frames(), mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route('/audio_feed')
def audio_feed():
    # 오디오 스트림을 위한 코드
    return Response(stream_with_context(generate_audio()), mimetype='multipart/x-mixed-replace; boundary=audio')

@app.route('/upload_frame', methods=['POST'])
def upload_frame():
    global frame_buffer
    if request.content_type == 'image/jpeg':
        frame = np.frombuffer(request.data, dtype=np.uint8)
        frame_buffer = cv2.imdecode(frame, cv2.IMREAD_COLOR)
        print(f"Received frame of size {len(request.data)} bytes")
        return 'Frame received', 200
    return 'Invalid content type', 400

@app.route('/upload_audio', methods=['POST'])
def upload_audio():
    global audio_buffer
    if request.content_type == 'audio/aac':
        audio_buffer.write(request.data)
        print(f"Received audio of size {len(request.data)} bytes")
        return 'Audio received', 200
    return 'Invalid content type', 400

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)

