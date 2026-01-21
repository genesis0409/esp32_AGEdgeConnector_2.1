#include <Arduino.h>
#include "I2C_Master.h"
#include "SHTSensor.h"

// I2C 마스터 인스턴스 생성 (Wire.h에 의존하지 않음)
I2C_Master i2c;

// Sensor with normal i2c address
// Sensor 1 with address pin pulled to GND
SHTSensor sht1(SHTSensor::SHT3X);

void setup()
{
    // Serial 먼저 초기화 (디버그 메시지 출력용)
    Serial.begin(115200);
    delay(1000);

    Serial.println("=== SHT Standalone Example ===");

    // I2C 마스터 초기화 (SDA: GPIO 13, SCL: GPIO 14)
    // arduino-sht 예제의 Wire.begin(13, 14)와 동일
    Serial.println("I2C 초기화 중...");
    if (!i2c.begin(13, 14, 100000, 0)) // 100kHz로 낮춤
    {
        Serial.println("I2C 초기화 실패!");
        while (1)
            delay(1000);
    }
    Serial.println("I2C 초기화 완료");

    // I2C 버스 스캔
    Serial.println("I2C 버스 스캔 중...");
    uint8_t found_addresses[16];
    int found = i2c.scan(found_addresses, 16);
    Serial.printf("발견된 디바이스: %d개\n", found);
    for (int i = 0; i < found; i++)
    {
        Serial.printf("  - 0x%02X\n", found_addresses[i]);
    }

    if (found == 0)
    {
        Serial.println("경고: I2C 디바이스가 발견되지 않음!");
        Serial.println("확인사항:");
        Serial.println("  1. SDA/SCL 핀 연결 (13/14)");
        Serial.println("  2. 센서 전원 (3.3V/GND)");
        Serial.println("  3. 풀업 저항 (4.7k~10k)");
    }

    // SHT 센서 초기화
    Serial.println("SHT 센서 초기화 중...");
    if (!sht1.init(i2c))
    {
        Serial.println("SHT 센서 초기화 실패!");
        Serial.println("센서가 응답하지 않습니다.");
        while (1)
            delay(1000);
    }

    Serial.println("SHT 센서 초기화 완료!");
    Serial.println("==============================");
}

void loop()
{
    if (sht1.readSample())
    {
        Serial.print("SHT1:\n");
        Serial.print("  RH: ");
        Serial.print(sht1.getHumidity(), 2);
        Serial.print(" %\n");
        Serial.print("  T:  ");
        Serial.print(sht1.getTemperature(), 2);
        Serial.print(" C\n");
    }
    else
    {
        Serial.println("Sensor 1: Error in readSample()");
    }

    delay(1000);
}
