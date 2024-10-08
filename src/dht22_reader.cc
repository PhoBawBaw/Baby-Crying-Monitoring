#include <wiringPi.h>
#include <iostream>
#include <libpq-fe.h>
#include <ctime>

#define MAX_TIMINGS 85  // 타이밍 제한
#define DHT_PIN 17      // GPIO 17번 핀 사용

int data[5] = {0, 0, 0, 0, 0}; // 5바이트 데이터(온습도 + 체크섬)

// PostgreSQL에 데이터를 삽입하는 함수
void insertDataToDB(double temperature, double humidity) {
    // PostgreSQL 데이터베이스 연결 정보
    const char* conninfo = "host=183.104.150.59 port=15432 dbname=rpi user=test password=1234";
    
    // PostgreSQL 연결 시도
    PGconn* conn = PQconnectdb(conninfo);

    // 연결 상태 확인
    if (PQstatus(conn) != CONNECTION_OK) {
        std::cerr << "Connection to database failed: " << PQerrorMessage(conn) << std::endl;
        PQfinish(conn);
        return;
    }

    // 현재 시간 가져오기
    std::time_t t = std::time(nullptr);
    char timeStr[100];
    std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", std::localtime(&t));

    // 데이터베이스에 온도와 습도, 시간을 저장할 SQL 쿼리 작성
    std::string query = "INSERT INTO zzanggu.environment (timestamp, temperature, humidity) VALUES ('" 
                        + std::string(timeStr) + "', "
                        + std::to_string(temperature) + ", " 
                        + std::to_string(humidity) + ");";

    // SQL 쿼리 실행
    PGresult* res = PQexec(conn, query.c_str());

    // 결과 확인
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "INSERT command failed: " << PQerrorMessage(conn) << std::endl;
    } else {
        std::cout << "Data inserted successfully!" << std::endl;
    }

    // 메모리 해제 및 연결 종료
    PQclear(res);
    PQfinish(conn);
}

// DHT22에서 데이터를 읽는 함수
void read_dht_data() {
    uint8_t laststate = HIGH;
    uint8_t counter = 0;
    uint8_t j = 0, i;
    data[0] = data[1] = data[2] = data[3] = data[4] = 0; // 데이터 배열 초기화

    // DHT 핀을 출력 모드로 설정하여 시작 신호를 보냄
    pinMode(DHT_PIN, OUTPUT);
    digitalWrite(DHT_PIN, LOW);
    delay(18);  // 최소 18ms 동안 LOW

    // 핀을 HIGH로 설정한 후, 40마이크로초 대기
    digitalWrite(DHT_PIN, HIGH);
    delayMicroseconds(40);

    // 입력 모드로 전환하여 센서 응답 대기
    pinMode(DHT_PIN, INPUT);

    // 센서 데이터 읽기
    for (i = 0; i < MAX_TIMINGS; i++) {
        counter = 0;
        while (digitalRead(DHT_PIN) == laststate) {
            counter++;
            delayMicroseconds(1);
            if (counter == 255) {
                break;
            }
        }
        laststate = digitalRead(DHT_PIN);

        if (counter == 255) break;

        // 첫 3 바이트는 무시
        if ((i >= 4) && (i % 2 == 0)) {
            data[j / 8] <<= 1;
            if (counter > 16) {
                data[j / 8] |= 1;
            }
            j++;
        }
    }

    // 체크섬 확인
    if ((j >= 40) && (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))) {
        float h = (float)((data[0] << 8) + data[1]) / 10.0; // 습도 데이터
        float c = (float)(((data[2] & 0x7F) << 8) + data[3]) / 10.0; // 온도 데이터

        if (data[2] & 0x80) c = -c;  // 음수 온도 처리

        std::cout << "Humidity = " << h << " % Temperature = " << c << " *C" << std::endl;

        // 데이터베이스에 온도와 습도 삽입
        insertDataToDB(c, h);
    } else {
        std::cout << "Data not good, skip" << std::endl;
    }
}

int main() {
    // WiringPi GPIO 설정
    if (wiringPiSetupGpio() == -1) {
        std::cerr << "Failed to initialize wiringPi" << std::endl;
        return 1;
    }

    while (1) {
        read_dht_data();
        delay(2000); // 2초마다 데이터 읽기
    }

    return 0;
}
