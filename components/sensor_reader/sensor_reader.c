#include <string.h>
#include "esp_log.h"

#include "sensor_reader.h"
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

#define RESPONSE_AHT20_HUMIDITY_OFFSET 0
#define RESPONSE_AHT20_TEMPERATURE_OFFSET 2

static const char* TAG = "SENSOR_READER";

static esp_err_t AHT20_get_status(sensor_reader_handle_t sr) {
    esp_err_t err = i2c_master_transmit_receive(
        sr->dev_AHT20,
        sr->cmd_buf + WRITE_BUF_AHT20_GET_STATUS_OFFSET,
        AHT20_GET_STATUS_CMD_LEGNTH,
        sr->read_buf,
        AHT20_STATUS_RESPONSE_LENGTH,
        10
    );
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Timeout while getting status from AHT20");
    }
    return err;
}

static esp_err_t AHT20_init(sensor_reader_handle_t sr) {
    esp_err_t err = i2c_master_transmit(
        sr->dev_AHT20,
        sr->cmd_buf + WRITE_BUF_AHT20_INIT_OFFSET,
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

static esp_err_t AHT20_get_measurement(sensor_reader_handle_t sr) {
    esp_err_t err = i2c_master_transmit_receive(
        sr->dev_AHT20,
        sr->cmd_buf + WRITE_BUF_AHT20_GET_MEASUREMENT_OFFSET,
        AHT20_GET_MEASUREMENT_CMD_LENGTH,
        sr->read_buf,
        AHT20_GET_MEASUREMENT_RESPONSE_LENGTH,
        AHT20_WAIT_MEASUREMENT_MS
    );

    ESP_LOGI(TAG, "%x, %x, %x, %x, %x, %x, %x, %x", sr->read_buf[0],
        sr->read_buf[1],
        sr->read_buf[2],
        sr->read_buf[3],
        sr->read_buf[4],
        sr->read_buf[5],
        sr->read_buf[6],
        sr->read_buf[7]);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Timeout when asking for measurement from AHT20");
    }

    return err;
}

static void AHT20_parse_measurement(sensor_reader_handle_t sr) {
    uint32_t raw_humidity = 0;
    uint32_t raw_temperature = 0;

    // raw humidity is first 20 bits of measurement data
    raw_humidity |= (((uint32_t)sr->read_buf[RESPONSE_AHT20_HUMIDITY_OFFSET]) << 12);
    raw_humidity |= (((uint32_t)sr->read_buf[RESPONSE_AHT20_HUMIDITY_OFFSET + 1]) << 4);
    raw_humidity |= (((uint32_t)sr->read_buf[RESPONSE_AHT20_HUMIDITY_OFFSET + 2]) >> 4);

    // raw temperature is second 20 bits of measurement data
    raw_humidity |= (((uint32_t)sr->read_buf[RESPONSE_AHT20_TEMPERATURE_OFFSET]) << 16);
    raw_humidity |= (((uint32_t)sr->read_buf[RESPONSE_AHT20_TEMPERATURE_OFFSET + 1]) << 8);
    raw_humidity |= (((uint32_t)sr->read_buf[RESPONSE_AHT20_TEMPERATURE_OFFSET + 2]));

    sr->measurement_hum = (raw_humidity / (1 << 20)) * 100;
    sr->measurement_temp = (raw_temperature / (1 << 20)) * 200 - 50;
}

static void setup_write_buf(sensor_reader_handle_t sr) {
    sr->cmd_buf[WRITE_BUF_AHT20_INIT_OFFSET + 0] = AHT20_INIT_CMD;
    sr->cmd_buf[WRITE_BUF_AHT20_INIT_OFFSET + 1] = AHT20_INIT_PARAM1;
    sr->cmd_buf[WRITE_BUF_AHT20_INIT_OFFSET + 2] = AHT20_INIT_PARAM2;
    sr->cmd_buf[WRITE_BUF_AHT20_GET_STATUS_OFFSET + 0] = AHT20_GET_STATUS_CMD;
    sr->cmd_buf[WRITE_BUF_AHT20_GET_MEASUREMENT_OFFSET + 0] = AHT20_GET_MEASUREMENT_CMD;
    sr->cmd_buf[WRITE_BUF_AHT20_GET_MEASUREMENT_OFFSET + 1] = AHT20_GET_MEASUREMENT_PARAM1;
    sr->cmd_buf[WRITE_BUF_AHT20_GET_MEASUREMENT_OFFSET + 2] = AHT20_GET_MEASUREMENT_PARAM2;
}

void init_sensor_reader(sensor_reader_handle_t sr, EventGroupHandle_t joy_eg) {
    sr->eg = joy_eg;

    memset(sr->read_buf, 0, SENSOR_READER_READ_BUF_SIZE);
    memset(sr->cmd_buf, 0, SENSOR_READER_WRITE_BUF_SIZE);

    setup_write_buf(sr);

    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_SDA_GPIO,
        .scl_io_num = I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = GLITCH_PERIOD_100KHZ,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &(sr->bus)));

    i2c_device_config_t dev_AHT20_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = DEV_AHT20_ADDR,
        .scl_speed_hz = 100000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(sr->bus, &dev_AHT20_config, &(sr->dev_AHT20)));
    
    vTaskDelay(pdMS_TO_TICKS(AHT20_WAIT_AFTER_POWER_ON_MS));

    AHT20_init(sr);
    for ( ;; ) {
        if (AHT20_get_status(sr) == ESP_OK) {
            if (sr->read_buf[0] & AHT20_STATUS_CALIBRATED_BIT) {
                ESP_LOGI(TAG, "AHT20 is calibrated!");
                ESP_LOGI(TAG, "%x", sr->read_buf[0]);
                break;
            }
            else {
                ESP_LOGW(TAG, "AHT20 is NOT calibrated!");
            }
        }
    }
}

static void event_loop(void* pvParameters) {
    sensor_reader_handle_t sr = pvParameters;
    EventBits_t bits = 0;

    for ( ;; ) {
        bits = xEventGroupWaitBits(
            sr->eg,
            JOYSTICK_SW_PRESSED_BIT,
            pdTRUE,
            pdTRUE,
            portMAX_DELAY
        );

        if (bits & JOYSTICK_SW_PRESSED_BIT) {
            AHT20_get_measurement(sr);
            AHT20_parse_measurement(sr);
            ESP_LOGI(TAG, "Humidity: %i | Temperature: %i", sr->measurement_hum, sr->measurement_temp);
        }
    }
}

void sensor_reader_start_event_loop(sensor_reader_handle_t sr) {
    xTaskCreate(
        event_loop,
        "SENSOR_READER",
        4096,
        sr,
        3,
        &(sr->task)
    );
}