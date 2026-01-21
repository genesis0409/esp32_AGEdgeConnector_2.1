#include "I2C_Master.h"

I2C_Master::I2C_Master() : i2c_port_(I2C_NUM_0), initialized_(false), freq_hz_(400000)
{
}

bool I2C_Master::begin(uint8_t sda_pin, uint8_t scl_pin, uint32_t freq_hz, uint8_t i2c_num)
{
    if (initialized_)
    {
        end();
    }

    i2c_port_ = (i2c_num == 0) ? I2C_NUM_0 : I2C_NUM_1;
    freq_hz_ = freq_hz;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = (gpio_num_t)sda_pin,
        .scl_io_num = (gpio_num_t)scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master = {
            .clk_speed = freq_hz,
        },
        .clk_flags = 0,
    };

    esp_err_t err = i2c_param_config(i2c_port_, &conf);
    if (err != ESP_OK)
    {
        return false;
    }

    err = i2c_driver_install(i2c_port_, conf.mode, 0, 0, 0);
    if (err != ESP_OK)
    {
        return false;
    }

    initialized_ = true;
    return true;
}

void I2C_Master::end()
{
    if (initialized_)
    {
        i2c_driver_delete(i2c_port_);
        initialized_ = false;
    }
}

bool I2C_Master::write(uint8_t address, const uint8_t *data, size_t len)
{
    if (!initialized_)
    {
        return false;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, (uint8_t *)data, len, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(i2c_port_, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    return ret == ESP_OK;
}

bool I2C_Master::read(uint8_t address, uint8_t *data, size_t len)
{
    if (!initialized_)
    {
        return false;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_READ, true);
    if (len > 1)
    {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(i2c_port_, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    return ret == ESP_OK;
}

bool I2C_Master::writeRead(uint8_t address, const uint8_t *write_data, size_t write_len,
                           uint8_t *read_data, size_t read_len)
{
    if (!initialized_)
    {
        return false;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, (uint8_t *)write_data, write_len, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_READ, true);
    if (read_len > 1)
    {
        i2c_master_read(cmd, read_data, read_len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, read_data + read_len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(i2c_port_, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    return ret == ESP_OK;
}

bool I2C_Master::writeReadDelayed(uint8_t address, const uint8_t *cmd_data, size_t cmd_len,
                                  uint8_t *data, size_t data_len, uint32_t delay_ms)
{
    if (!initialized_)
    {
        return false;
    }

    // 명령 전송
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, (uint8_t *)cmd_data, cmd_len, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(i2c_port_, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK)
    {
        return false;
    }

    // 대기 시간
    if (delay_ms > 0)
    {
        delay(delay_ms);
    }

    // 데이터 읽기
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_READ, true);
    if (data_len > 1)
    {
        i2c_master_read(cmd, data, data_len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + data_len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(i2c_port_, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    return ret == ESP_OK;
}

int I2C_Master::scan(uint8_t *found_addresses, size_t max_count)
{
    if (!initialized_)
    {
        return 0;
    }

    int found = 0;
    for (uint8_t addr = 1; addr < 127 && found < max_count; addr++)
    {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);

        esp_err_t ret = i2c_master_cmd_begin(i2c_port_, cmd, pdMS_TO_TICKS(50));
        i2c_cmd_link_delete(cmd);

        if (ret == ESP_OK)
        {
            found_addresses[found++] = addr;
        }
    }

    return found;
}