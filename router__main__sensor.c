#include <stdint.h>
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sensor.h"

#define SENSOR_SDA_GPIO GPIO_NUM_21
#define SENSOR_SCL_GPIO GPIO_NUM_22
#define SENSOR_ADDRESS 0x44
#define SENSOR_I2C_SPEED 100000
#define SENSOR_TIMEOUT_MS 100

static i2c_master_bus_handle_t bus_handle = NULL;
static i2c_master_dev_handle_t dev_handle = NULL;

esp_err_t sensor_init(void)
{
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = SENSOR_SDA_GPIO,
        .scl_io_num = SENSOR_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    esp_err_t err = i2c_new_master_bus(&bus_cfg, &bus_handle);
    if (err != ESP_OK) {
        return err;
    }
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SENSOR_ADDRESS,
        .scl_speed_hz = SENSOR_I2C_SPEED,
    };
    return i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
}

esp_err_t sensor_read(float *temperature, float *humidity)
{
    uint8_t command[2] = {0x24, 0x00};
    uint8_t data[6] = {0};

    esp_err_t err = i2c_master_transmit(dev_handle, command, sizeof(command), SENSOR_TIMEOUT_MS);
    if (err != ESP_OK) {
        return err;
    }
    vTaskDelay(pdMS_TO_TICKS(20));
    err = i2c_master_receive(dev_handle, data, sizeof(data), SENSOR_TIMEOUT_MS);
    if (err != ESP_OK) {
        return err;
    }
    uint16_t raw_temp = ((uint16_t)data[0] << 8) | data[1];
    uint16_t raw_hum = ((uint16_t)data[3] << 8) | data[4];
    *temperature = -45.0f + 175.0f * ((float)raw_temp / 65535.0f);
    *humidity = 100.0f * ((float)raw_hum / 65535.0f);
    return ESP_OK;
}
