#include "spi_manager.h"

#include "esp_log.h"
#include "joystick_reader.h"

#define MISO_GPIO 2
#define MOSI_GPIO 42
#define CS_GPIO 41
#define SCL_GPIO 40

#define LSM6DS3_ADDR_WHO_AM_I 0x0F
#define LSM6DS3_ADDR_CTRL1_XL 0x10
#define LSM6DS3_ADDR_OUTX_L_XL 0x28
#define LSM6DS3_ADDR_OUTY_L_XL 0x2A
#define LSM6DS3_ADDR_OUTZ_L_XL 0x2C

// how many bytes device returns when asked for data from any axis
#define LSM6DS3_XL_DATA_OUT_SIZE 2

// output data rate for 12.5 Hz
#define LSM6DS3_ODR12_5_XL 0x10

#define LSM6DS3_WHO_AM_I_VALUE 0x69
#define LSM6DS3_READ_BIT 0x80

static const char* TAG = "SPI_MANAGER";

esp_err_t lsm6ds3_read_reg(spi_manager_handle_t sm, uint8_t reg_addr, size_t size) {
    if (sm == NULL || sm->spi == NULL || size == 0) return ESP_ERR_INVALID_ARG;

    reg_addr |= LSM6DS3_READ_BIT;
    sm->tx_buf[0] = reg_addr;

    spi_transaction_t trans = {
        .rx_buffer = sm->rx_buf,
        .tx_buffer = sm->tx_buf,
        .length = (1 + size) * 8,
    };
    return spi_device_transmit(sm->spi, &trans);
}

esp_err_t lsm6ds3_write_reg_byte(spi_manager_handle_t sm, uint8_t reg_addr, uint8_t data) {
    if (sm == NULL || sm->spi == NULL) return ESP_ERR_INVALID_ARG;

    sm->tx_buf[0] = reg_addr;
    sm->tx_buf[1] = data;

    spi_transaction_t trans = {
        .tx_buffer = sm->tx_buf,
        .length = 2 * 8,
    };
    spi_device_transmit(sm->spi, &trans);

    return ESP_OK;
}

esp_err_t lsm6ds3_write_reg_data(spi_manager_handle_t sm, uint8_t reg_addr, const uint8_t* data, size_t data_size) {
    if (sm == NULL || sm->spi == NULL || data == NULL || data_size == 0) return ESP_ERR_INVALID_ARG;

    sm->tx_buf[0] = reg_addr;
    memmove(sm->tx_buf + 1, data, data_size);

    spi_transaction_t trans = {
        .tx_buffer = sm->tx_buf,
        .length = (1 + data_size) * 8,
    };
    spi_device_transmit(sm->spi, &trans);

    return ESP_OK;
}

esp_err_t lsm6ds3_init_xl(spi_manager_handle_t sm) {
    if (sm == NULL || sm->spi == NULL) {
        ESP_LOGE(TAG, "function got NULL pointer as a parameter");
        return ESP_ERR_INVALID_ARG;
    }

    
    esp_err_t err = lsm6ds3_read_reg(sm, LSM6DS3_ADDR_WHO_AM_I, 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error while transmitting SPI");
        return err;
    }
    vTaskDelay(pdMS_TO_TICKS(100));
    printf("rx buf[1]: %x\n", sm->rx_buf[1]);
    if (sm->rx_buf[1] != LSM6DS3_WHO_AM_I_VALUE) {
        ESP_LOGE(TAG, "LSM6DS3 did not confirm its WHO_AM_I");
        return ESP_ERR_TIMEOUT;
    }

    lsm6ds3_write_reg_byte(sm, LSM6DS3_ADDR_CTRL1_XL, LSM6DS3_ODR12_5_XL);

    return ESP_OK;
}

static void event_loop(void* pvParameters) {
    spi_manager_handle_t sm = pvParameters;
    EventBits_t bits;
    int x, y, z;

    for ( ;; ) {
        bits = xEventGroupWaitBits(
            sm->eg,
            JOYSTICK_SW_PRESSED_BIT,
            pdTRUE,
            pdTRUE,
            portMAX_DELAY
        );

        if (bits == JOYSTICK_SW_PRESSED_BIT) {
            lsm6ds3_read_reg(sm, LSM6DS3_ADDR_OUTX_L_XL, LSM6DS3_XL_DATA_OUT_SIZE * 3);
            x = ((int16_t)(sm->rx_buf[2]) << 8) | sm->rx_buf[1];
            y = ((int16_t)(sm->rx_buf[4]) << 8) | sm->rx_buf[3];
            z = ((int16_t)(sm->rx_buf[6]) << 8) | sm->rx_buf[5];
            ESP_LOGI(TAG, "Received info from accelerometer:");
            ESP_LOGI(TAG, "x: %i | y: %i | z: %i", x, y, z);
        }
    }
}

esp_err_t spi_manager_start_event_loop(spi_manager_handle_t sm) {
    if (sm == NULL) return ESP_ERR_INVALID_ARG;

    xTaskCreate(
        event_loop,
        "SPI_MANAGER",
        2048,
        sm,
        3,
        &(sm->task)
    );

    return ESP_OK;
}

esp_err_t init_spi_manager(spi_manager_handle_t sm, EventGroupHandle_t joy_eg) {
    if (sm == NULL || joy_eg == NULL) return ESP_ERR_INVALID_ARG;

    sm->eg = joy_eg;

    // sm->tx_buf = heap_caps_malloc(sizeof(uint8_t) * SPI_MANAGER_TXRX_BUF_SIZE, MALLOC_CAP_DMA);
    // sm->rx_buf = heap_caps_malloc(sizeof(uint8_t) * SPI_MANAGER_TXRX_BUF_SIZE, MALLOC_CAP_DMA);
    
    memset(sm->rx_buf, 0, SPI_MANAGER_TXRX_BUF_SIZE);
    memset(sm->tx_buf, 0, SPI_MANAGER_TXRX_BUF_SIZE);

    spi_bus_config_t bus_cfg = {
        .miso_io_num = MISO_GPIO,
        .mosi_io_num = MOSI_GPIO,
        .sclk_io_num = SCL_GPIO,
        .quadhd_io_num = -1,
        .quadwp_io_num = -1,
        .max_transfer_sz = 16,
    };

    esp_err_t err = spi_bus_initialize(SPI3_HOST, &bus_cfg, SPI_DMA_DISABLED);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus");
        return err;
    }

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 1000000,
        .mode = 3,
        .spics_io_num = CS_GPIO,
        .queue_size = 1,
    };
    spi_bus_add_device(SPI3_HOST, &dev_cfg, &(sm->spi));

    return ESP_OK;
}