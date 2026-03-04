#include <string.h>
#include "esp_log.h"

#include "i2c_manager.h"
#include "joystick_reader.h"

#define GLITCH_PERIOD_400KHZ 7
#define GLITCH_PERIOD_100KHZ 12
#define GLITCH_PERIOD_1MHZ 2

#define DEV_AHT20_ADDR 0x38

#define AHT20_INIT_CMD 0xBE
#define AHT20_INIT_PARAM1 0x08
#define AHT20_INIT_PARAM2 0x00
#define AHT20_INIT_CMD_LENGTH 3

#define AHT20_GET_STATUS_CMD 0x71
#define AHT20_GET_STATUS_CMD_LEGNTH 1
#define AHT20_STATUS_RESPONSE_LENGTH 1

#define AHT20_GET_MEASUREMENT_CMD 0xAC
#define AHT20_GET_MEASUREMENT_PARAM1 0x33
#define AHT20_GET_MEASUREMENT_PARAM2 0x00
#define AHT20_GET_MEASUREMENT_CMD_LENGTH 3
#define AHT20_GET_MEASUREMENT_RESPONSE_LENGTH 8

#define AHT20_WAIT_AFTER_POWER_ON_MS 45
#define AHT20_WAIT_AFTER_INIT_MS 15
#define AHT20_WAIT_MEASUREMENT_MS 85

#define AHT20_STATUS_CALIBRATED_BIT BIT3
#define AHT20_STATUS_BUSY_BIT BIT7

#define WRITE_BUF_AHT20_INIT_OFFSET 0
#define WRITE_BUF_AHT20_GET_STATUS_OFFSET 3
#define WRITE_BUF_AHT20_GET_MEASUREMENT_OFFSET 4

#define RESPONSE_AHT20_HUMIDITY_OFFSET 1
#define RESPONSE_AHT20_TEMPERATURE_OFFSET 3

static const char* TAG = "I2C_MANAGER";

static esp_err_t AHT20_get_status(i2c_manager_handle_t i2c_manager) {
    esp_err_t err = i2c_master_transmit_receive(
        i2c_manager->dev_AHT20,
        i2c_manager->cmd_buf + WRITE_BUF_AHT20_GET_STATUS_OFFSET,
        AHT20_GET_STATUS_CMD_LEGNTH,
        i2c_manager->read_buf,
        AHT20_STATUS_RESPONSE_LENGTH,
        10
    );
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Timeout while getting status from AHT20");
    }
    return err;
}

static esp_err_t AHT20_init(i2c_manager_handle_t i2c_manager) {
    esp_err_t err = i2c_master_transmit(
        i2c_manager->dev_AHT20,
        i2c_manager->cmd_buf + WRITE_BUF_AHT20_INIT_OFFSET,
        AHT20_INIT_CMD_LENGTH,
        10
    );
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Timeout when initing AHT20");
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(AHT20_WAIT_AFTER_INIT_MS));
    return err;
}

static esp_err_t AHT20_get_measurement(i2c_manager_handle_t i2c_manager) {
    esp_err_t err = i2c_master_transmit_receive(
        i2c_manager->dev_AHT20,
        i2c_manager->cmd_buf + WRITE_BUF_AHT20_GET_MEASUREMENT_OFFSET,
        AHT20_GET_MEASUREMENT_CMD_LENGTH,
        i2c_manager->read_buf,
        AHT20_GET_MEASUREMENT_RESPONSE_LENGTH,
        AHT20_WAIT_MEASUREMENT_MS
    );

    ESP_LOGI(TAG, "%x, %x, %x, %x, %x, %x, %x, %x", i2c_manager->read_buf[0],
        i2c_manager->read_buf[1],
        i2c_manager->read_buf[2],
        i2c_manager->read_buf[3],
        i2c_manager->read_buf[4],
        i2c_manager->read_buf[5],
        i2c_manager->read_buf[6],
        i2c_manager->read_buf[7]);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Timeout when asking for measurement from AHT20");
    }

    return err;
}

static void AHT20_parse_measurement(i2c_manager_handle_t i2c_manager) {
    uint32_t raw_humidity = 0;
    uint32_t raw_temperature = 0;

    // raw humidity is first 20 bits of measurement data
    raw_humidity |= (((uint32_t)i2c_manager->read_buf[RESPONSE_AHT20_HUMIDITY_OFFSET]) << 12);
    raw_humidity |= (((uint32_t)i2c_manager->read_buf[RESPONSE_AHT20_HUMIDITY_OFFSET + 1]) << 4);
    raw_humidity |= (((uint32_t)i2c_manager->read_buf[RESPONSE_AHT20_HUMIDITY_OFFSET + 2]) >> 4);

    // raw temperature is second 20 bits of measurement data
    raw_temperature |= ((((uint32_t)i2c_manager->read_buf[RESPONSE_AHT20_TEMPERATURE_OFFSET]) & 0x0f) << 16);
    raw_temperature |= (((uint32_t)i2c_manager->read_buf[RESPONSE_AHT20_TEMPERATURE_OFFSET + 1]) << 8);
    raw_temperature |= (((uint32_t)i2c_manager->read_buf[RESPONSE_AHT20_TEMPERATURE_OFFSET + 2]));

    i2c_manager->measurement_hum = ((float)raw_humidity / (1 << 20)) * 100;
    i2c_manager->measurement_temp = ((float)raw_temperature / (1 << 20)) * 200 - 50;
}

static void AHT20_setup_write_buf(i2c_manager_handle_t i2c_manager) {
    i2c_manager->cmd_buf[WRITE_BUF_AHT20_INIT_OFFSET + 0] = AHT20_INIT_CMD;
    i2c_manager->cmd_buf[WRITE_BUF_AHT20_INIT_OFFSET + 1] = AHT20_INIT_PARAM1;
    i2c_manager->cmd_buf[WRITE_BUF_AHT20_INIT_OFFSET + 2] = AHT20_INIT_PARAM2;
    i2c_manager->cmd_buf[WRITE_BUF_AHT20_GET_STATUS_OFFSET + 0] = AHT20_GET_STATUS_CMD;
    i2c_manager->cmd_buf[WRITE_BUF_AHT20_GET_MEASUREMENT_OFFSET + 0] = AHT20_GET_MEASUREMENT_CMD;
    i2c_manager->cmd_buf[WRITE_BUF_AHT20_GET_MEASUREMENT_OFFSET + 1] = AHT20_GET_MEASUREMENT_PARAM1;
    i2c_manager->cmd_buf[WRITE_BUF_AHT20_GET_MEASUREMENT_OFFSET + 2] = AHT20_GET_MEASUREMENT_PARAM2;
}

void init_i2c_manager(i2c_manager_handle_t i2c_manager, EventGroupHandle_t joy_eg) {
    i2c_manager->eg = joy_eg;

    memset(i2c_manager->read_buf, 0, SENSOR_READER_READ_BUF_SIZE);
    memset(i2c_manager->cmd_buf, 0, SENSOR_READER_WRITE_BUF_SIZE);

    AHT20_setup_write_buf(i2c_manager);

    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_SDA_GPIO,
        .scl_io_num = I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = GLITCH_PERIOD_100KHZ,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &(i2c_manager->bus)));

    i2c_device_config_t dev_AHT20_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = DEV_AHT20_ADDR,
        .scl_speed_hz = 100000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_manager->bus, &dev_AHT20_config, &(i2c_manager->dev_AHT20)));
    
    vTaskDelay(pdMS_TO_TICKS(AHT20_WAIT_AFTER_POWER_ON_MS));

    AHT20_init(i2c_manager);
    for ( ;; ) {
        if (AHT20_get_status(i2c_manager) == ESP_OK) {
            if (i2c_manager->read_buf[0] & AHT20_STATUS_CALIBRATED_BIT) {
                ESP_LOGI(TAG, "AHT20 is calibrated!");
                ESP_LOGI(TAG, "%x", i2c_manager->read_buf[0]);
                break;
            }
            else {
                ESP_LOGW(TAG, "AHT20 is NOT calibrated!");
            }
        }
    }
}

static void event_loop(void* pvParameters) {
    i2c_manager_handle_t i2c_manager = pvParameters;
    EventBits_t bits = 0;

    for ( ;; ) {
        bits = xEventGroupWaitBits(
            i2c_manager->eg,
            JOYSTICK_SW_PRESSED_BIT,
            pdTRUE,
            pdTRUE,
            portMAX_DELAY
        );

        if (bits & JOYSTICK_SW_PRESSED_BIT) {
            AHT20_get_measurement(i2c_manager);
            AHT20_parse_measurement(i2c_manager);
            ESP_LOGI(TAG, "Humidity: %i | Temperature: %i", i2c_manager->measurement_hum, i2c_manager->measurement_temp);
        }
    }
}

void i2c_manager_start_event_loop(i2c_manager_handle_t i2c_manager) {
    xTaskCreate(
        event_loop,
        "I2C_MANAGER",
        4096,
        i2c_manager,
        3,
        &(i2c_manager->task)
    );
}