#ifndef I2C_MASTER_H
#define I2C_MASTER_H

#include <Arduino.h>
#include <driver/i2c.h>
#include <esp_err.h>
/**
 * 독립적인 I2C 마스터 라이브러리
 * Wire.h에 의존하지 않는 ESP32 네이티브 I2C 드라이버 기반 구현
 */
class I2C_Master
{
public:
    I2C_Master();

    /**
     * I2C 마스터 초기화
     * @param sda_pin SDA 핀 번호
     * @param scl_pin SCL 핀 번호
     * @param freq_hz I2C 클럭 주파수 (기본값: 400000 = 400kHz)
     * @param i2c_num I2C 컨트롤러 번호 (0 또는 1)
     * @return 초기화 성공 여부
     */
    bool begin(uint8_t sda_pin, uint8_t scl_pin, uint32_t freq_hz = 400000, uint8_t i2c_num = 0);

    /**
     * I2C 마스터 해제
     */
    void end();

    /**
     * I2C 쓰기 트랜잭션
     * @param address 7-bit I2C 주소
     * @param data 전송할 데이터 버퍼
     * @param len 데이터 길이
     * @return 성공 여부
     */
    bool write(uint8_t address, const uint8_t *data, size_t len);

    /**
     * I2C 읽기 트랜잭션
     * @param address 7-bit I2C 주소
     * @param data 읽은 데이터를 저장할 버퍼
     * @param len 읽을 데이터 길이
     * @return 성공 여부
     */
    bool read(uint8_t address, uint8_t *data, size_t len);

    /**
     * I2C 쓰기 후 읽기 트랜잭션
     * @param address 7-bit I2C 주소
     * @param write_data 쓸 데이터 버퍼
     * @param write_len 쓸 데이터 길이
     * @param read_data 읽은 데이터를 저장할 버퍼
     * @param read_len 읽을 데이터 길이
     * @return 성공 여부
     */
    bool writeRead(uint8_t address, const uint8_t *write_data, size_t write_len,
                   uint8_t *read_data, size_t read_len);

    /**
     * I2C 쓰기 후 읽기 트랜잭션 (명령 전송 후 데이터 읽기)
     * @param address 7-bit I2C 주소
     * @param cmd 명령어 버퍼
     * @param cmd_len 명령어 길이
     * @param data 읽은 데이터를 저장할 버퍼
     * @param data_len 읽을 데이터 길이
     * @param delay_ms 명령 전송 후 대기 시간 (ms)
     * @return 성공 여부
     */
    bool writeReadDelayed(uint8_t address, const uint8_t *cmd, size_t cmd_len,
                          uint8_t *data, size_t data_len, uint32_t delay_ms = 0);

    /**
     * I2C 버스 스캔 (디버그용)
     * @param found_addresses 발견된 주소를 저장할 배열
     * @param max_count 배열 최대 크기
     * @return 발견된 디바이스 개수
     */
    int scan(uint8_t *found_addresses, size_t max_count);

    /**
     * I2C 마스터 핸들이 유효한지 확인
     */
    bool isValid() const { return initialized_; }

private:
    i2c_port_t i2c_port_;
    bool initialized_;
    uint32_t freq_hz_;
};

#endif // I2C_MASTER_H