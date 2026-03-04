#ifndef SPI_MANAGER_H
#define SPI_MANAGER_H

#include "driver/spi_master.h"

#define SPI_MANAGER_TXRX_BUF_SIZE 16

typedef struct {
    TaskHandle_t task;
    spi_device_handle_t spi;
    uint8_t rx_buf[SPI_MANAGER_TXRX_BUF_SIZE];
    uint8_t tx_buf[SPI_MANAGER_TXRX_BUF_SIZE];
    EventGroupHandle_t eg;
} spi_manager_t;

typedef spi_manager_t* spi_manager_handle_t;

esp_err_t init_spi_manager(spi_manager_handle_t sm, EventGroupHandle_t joy_eg);
esp_err_t spi_manager_start_event_loop(spi_manager_handle_t sm);
esp_err_t lsm6ds3_init_xl(spi_manager_handle_t sm);
esp_err_t lsm6ds3_write_reg_data(spi_manager_handle_t sm, uint8_t reg_addr, const uint8_t* data, size_t data_size);
esp_err_t lsm6ds3_write_reg_byte(spi_manager_handle_t sm, uint8_t reg_addr, uint8_t data);
esp_err_t lsm6ds3_read_reg(spi_manager_handle_t sm, uint8_t reg_addr, size_t size);

#endif // SPI_MANAGER_H