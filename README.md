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

3. `src/client` 디렉토리로 이동합니다:
   ```bash
   cd Baby-Crying-Monitoring/src/client
   ```

4. 클라이언트 코드를 컴파일합니다:
   ```bash
   g++ client_start.cc -o client_start -std=c++17 -lpthread
   ```

5. 클라이언트를 실행합니다:
   ```bash
   ./client_start
   ```

#### 중앙 서버

1. 움직임 감지에 필요한 파이썬 라이브러리를 설치합니다:
   ```bash
   pip install ultralytics
   pip install opencv-python
   pip install psycopg2-binary
   ```

2. 리포지토리를 클론합니다:
   ```bash
   git clone https://github.com/PhoBawBaw/Baby-Crying-Monitoring.git
   ```

3. `src/server` 디렉토리로 이동합니다:
   ```bash
   cd Baby-Crying-Monitoring/src/server
   ```

4. 서버 코드를 컴파일합니다:
   ```bash
   g++ server_start.cc -o server_start -std=c++17 -lpthread
   ```

5. 서버를 실행합니다:
   ```bash
   ./server_start
   ```

6. 녹화 영상, 오디오 파일은 `/baby` 디렉토리에서 확인합니다:
   ```bash
   cd /baby
   ```

### Notes

- 서버 주소와 포트 번호는 소스 코드에서 직접 수정해야 합니다.

- 시스템을 빌드하기 전에 웹앱과 모델 컨테이너가 모두 작동 중인지 확인하세요.

## License
이 리포지토리는 [AGPL-3.0 라이선스](https://github.com/PhoBawBaw/Baby-Crying-Monitoring/blob/main/LICENSE.md)를 따릅니다.
