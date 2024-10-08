# 컴파일러 설정
CXX = g++
CXXFLAGS = -Wall -std=c++11

# 패키지 설정
GST_FLAGS = $(shell pkg-config --cflags --libs gstreamer-1.0 gstreamer-rtsp-server-1.0)
DHT_LIBS = -lwiringPi -lpq

# 타겟 바이너리 파일
CLIENT_TARGET = ~/Baby-Crying-Monitoring/src/Client
SERVER_TARGET = ~/Baby-Crying-Monitoring/src/Server
DHT_TARGET = ~/Baby-Crying-Monitoring/src/dht22_reader

# 소스 파일
CLIENT_SRC = ~/Baby-Crying-Monitoring/src/Client.cc
SERVER_SRC = ~/Baby-Crying-Monitoring/src/Server.cc
DHT_SRC = ~/Baby-Crying-Monitoring/src/dht22_reader.cc

# 기본 빌드 대상
all: $(CLIENT_TARGET) $(DHT_TARGET) $(SERVER_TARGET)

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

