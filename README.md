# 아기 울음 소리 모니터링 시스템

## Introduction
본 프로젝트는 아기의 상태를 실시간으로 모니터링하고, 울음 소리를 분석하여 부모에게 알림을 제공하는 시스템을 구현하기 위한 프로젝트입니다. 이 리포지토리는 시스템 구현에 관한 내용을 다룹니다.

## Usage

### Configuration
본 시스템을 구현하기 위해서는 가정용 라즈베리파이 5와 중앙 서버(워크스테이션, 관리자용)가 필요합니다. 실시간 비디오 스트리밍을 위해 GStreamer, RTSP, HLS가 사용됩니다.

### Setup

#### 라즈베리파이 5 (가정용)

1. 홈 디렉토리로 이동합니다:
   ```bash
   cd ~/
   ```

2. 리포지토리를 클론합니다:
   ```bash
   git clone https://github.com/PhoBawBaw/Baby-Crying-Monitoring.git
   ```

3. `run` 디렉토리로 이동합니다:
   ```bash
   cd Baby-Crying-Monitoring/run
   ```

4. 클라이언트 코드를 컴파일합니다:
   ```bash
   g++ client.cc -o client -std=c++17 -lpthread
   ```

5. 클라이언트를 실행합니다:
   ```bash
   ./client
   ```

#### 중앙 서버

1. 홈 디렉토리로 이동합니다:
   ```bash
   cd ~/
   ```

2. 리포지토리를 클론합니다:
   ```bash
   git clone https://github.com/PhoBawBaw/Baby-Crying-Monitoring.git
   ```

3. 서버 코드를 백엔드 컨테이너로 복사합니다:
   ```bash
   docker cp run/server.cc <백엔드 컨테이너>:/app
   docker cp src/predict.py <백엔드 컨테이너>:/app
   ```

- 백엔드 컨테이너 이름은 다음 명령어로 확인할 수 있습니다:
  ```bash
  docker ps | grep web_app_api
  ```

5. 백엔드 컨테이너에 접속합니다:
   ```bash
   sudo docker exec -it <백엔드 컨테이너> /bin/bash
   ```

6. `app` 디렉토리로 이동합니다:
   ```bash
   cd /app
   ```

7. 서버 코드를 컴파일합니다:
   ```bash
   g++ server.cc -o server -std=c++17 -lpthread
   ```

8. 서버를 실행합니다:
   ```bash
   ./server
   ```

### Notes

- 서버 주소와 포트 번호는 소스 코드에서 직접 수정해야 합니다.

- 시스템을 빌드하기 전에 웹앱과 모델 컨테이너가 모두 작동 중인지 확인하세요.

## License
TBD

---
