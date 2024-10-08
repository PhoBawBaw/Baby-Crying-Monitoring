# 컴파일러 설정
CXX = g++
CXXFLAGS = -Wall -std=c++11

# 패키지 설정
GST_FLAGS = $(shell pkg-config --cflags --libs gstreamer-1.0 gstreamer-rtsp-server-1.0)
DHT_LIBS = -lwiringPi -lpq

# 타겟 바이너리 파일 (bin 디렉터리에 바이너리 저장)
BINDIR = $(HOME)/Baby-Crying-Monitoring/bin
CLIENT_TARGET = $(BINDIR)/Client
SERVER_TARGET = $(BINDIR)/Server
DHT_TARGET = $(BINDIR)/dht22_reader

# 소스 파일
SRCDIR = $(HOME)/Baby-Crying-Monitoring/src
CLIENT_SRC = $(SRCDIR)/Client.cc
SERVER_SRC = $(SRCDIR)/Server.cc
DHT_SRC = $(SRCDIR)/dht22_reader.cc

# 기본 빌드 대상
all: $(BINDIR) $(CLIENT_TARGET) $(DHT_TARGET) $(SERVER_TARGET)

# 디렉터리 생성
$(BINDIR):
	mkdir -p $(BINDIR)

# Client 빌드
$(CLIENT_TARGET): $(CLIENT_SRC)
	$(CXX) $(CXXFLAGS) -o $(CLIENT_TARGET) $(CLIENT_SRC) $(GST_FLAGS)

# dht22_reader 빌드
$(DHT_TARGET): $(DHT_SRC)
	$(CXX) $(CXXFLAGS) -o $(DHT_TARGET) $(DHT_SRC) $(DHT_LIBS)

# Server 빌드
$(SERVER_TARGET): $(SERVER_SRC)
	$(CXX) $(CXXFLAGS) -o $(SERVER_TARGET) $(SERVER_SRC)

# 클린 명령어 (바이너리 파일 삭제)
clean:
	rm -f $(CLIENT_TARGET) $(DHT_TARGET) $(SERVER_TARGET)
