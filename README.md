# gRPC 기반 영상, 음성, 온습도 데이터 전송

## 0. 테스트 환경
- **Client**: Raspberry Pi 5 (Bookworm OS)
- **Server**: Ubuntu 20.04.6 LTS

## 1. 개발 환경 설정

### 필수 개발 도구 설치

시스템에 필요한 기본 개발 도구를 설치합니다:

- **build-essential**: C/C++ 컴파일러 및 빌드 도구(예: `gcc`, `g++`, `make`)
- **autoconf**: 설정 스크립트를 생성하는 도구 (`./configure` 스크립트 생성)
- **libtool**: 플랫폼 간 공유 라이브러리 생성 및 사용 스크립트
- **cmake**: 크로스 플랫폼 빌드 시스템

```bash
sudo apt-get update
sudo apt-get install build-essential autoconf libtool cmake
```

### gRPC 빌드 및 테스트를 위한 추가 라이브러리 설치

- **libgflags-dev**: C++ 플래그 처리 라이브러리
- **libgtest-dev**: Google 테스트 라이브러리 (단위 테스트 프레임워크)
- **clang**: C/C++ 컴파일러
- **libc++-dev**: C++ 표준 라이브러리
- **libssl-dev**: SSL/TLS 암호화 라이브러리
- **libre2-dev**: 정규표현식 라이브러리
- **pkg-config**: 라이브러리 컴파일 및 링크 플래그 관리 도구

```bash
sudo apt-get install libgflags-dev libgtest-dev clang libc++-dev libssl-dev libre2-dev pkg-config
```

## 2. gRPC 및 의존성 라이브러리 설치

### gRPC 설치

gRPC와 필요한 서브모듈을 클론하고 초기화합니다:

```bash
git clone -b v1.65.0 https://github.com/grpc/grpc
cd grpc
git submodule update --init --recursive
```

### gRPC 및 Protocol Buffers 빌드

gRPC와 Protocol Buffers를 빌드하고 설치합니다:

```bash
# gRPC 빌드
mkdir -p cmake/build && cd cmake/build
cmake ../..
sudo make -j$(nproc)
sudo make install
cd ../../

# Protocol Buffers 빌드
cd third_party/protobuf
mkdir -p cmake/build && cd cmake/build
cmake ../..
sudo make -j$(nproc)
sudo make install
```

### 추가 라이브러리 설치

```bash
# Benchmark 설치
cd third_party/benchmark
cmake -E make_directory "build"
cmake -E chdir "build" cmake -DBENCHMARK_DOWNLOAD_DEPENDENCIES=on -DCMAKE_BUILD_TYPE=Release ../
cmake --build "build" --config Release
sudo cmake --build "build" --config Release --target install

# Abseil 테스트 실행
cd third_party/abseil-cpp
mkdir build
cd build
cmake -DABSL_BUILD_TESTING=ON -DABSL_USE_GOOGLETEST_HEAD=ON ..
make -j$(nproc)
ctest
```

## 3. 환경 변수 설정

패키지 구성 파일을 찾을 수 있도록 `PKG_CONFIG_PATH`를 설정합니다:

```bash
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig
```

## 4. 프로젝트 설정

### 프로젝트 클론

개인 액세스 토큰을 생성한 후, 프로젝트를 클론합니다:

```bash
git clone https://<YOUR_ACCESS_TOKEN>@github.com/PhoBawBaw/Baby-Crying-Monitoring.git
```

### 프로젝트 구조

다음은 프로젝트의 기본 디렉토리 구조입니다:

```
project_root/
├── proto/
│   └── helloworld.proto
├── src/
│   ├── greeter_server.cc
│   ├── greeter_client.cc
│   ├── helloworld.pb.cc
│   ├── helloworld.pb.h
│   ├── helloworld.grpc.pb.cc
│   └── helloword.grpc.pb.h
└── Makefile
```

## 5. 프로토콜 파일 컴파일

생성한 `helloworld.proto` 파일을 컴파일하여 C++ 코드를 생성합니다:

```bash
# gRPC 및 Protocol Buffers 컴파일
protoc -I=proto --grpc_out=./src/ --plugin=protoc-gen-grpc=/usr/local/bin/grpc_cpp_plugin proto/helloworld.proto
protoc -I=proto --cpp_out=./src proto/helloworld.proto
```

## 6. 컴파일

Makefile을 사용하여 프로젝트를 컴파일합니다:

```bash
make -j$(nproc)
```

## 7. 실행

서버와 클라이언트를 각각 실행합니다:

```bash
# 서버 실행
./greeter_server

# 클라이언트 실행
./greeter_client
```
