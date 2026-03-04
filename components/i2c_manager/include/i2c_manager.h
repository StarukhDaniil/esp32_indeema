#ifndef I2C_MANAGER_H
#define I2C_MANAGER_H

#include "driver/i2c_master.h"

#define I2C_SDA_GPIO 16
#define I2C_SCL_GPIO 17

#define SENSOR_READER_WRITE_BUF_SIZE 16
#define SENSOR_READER_READ_BUF_SIZE 16

#include "esp_event.h"

typedef struct {
    TaskHandle_t task;
    EventGroupHandle_t eg;
    i2c_master_bus_handle_t bus;
    i2c_master_dev_handle_t dev_BMP280;
    i2c_master_dev_handle_t dev_AHT20;
    uint8_t cmd_buf[SENSOR_READER_WRITE_BUF_SIZE];
    uint8_t read_buf[SENSOR_READER_READ_BUF_SIZE];
    int16_t measurement_hum;
    int16_t measurement_temp;
} i2c_manager_t;

typedef i2c_manager_t* i2c_manager_handle_t;

void init_i2c_manager(i2c_manager_handle_t i2c_manager, EventGroupHandle_t joy_eg);
void i2c_manager_start_event_loop(i2c_manager_handle_t i2c_manager);

#endif // I2C_MANAGER_H