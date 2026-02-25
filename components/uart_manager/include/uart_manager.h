#ifndef UART_MANAGER_H
#define UART_MANAGER_H

#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"

typedef struct {
    size_t rx_buf_size;
    uart_port_t port;
    uint8_t tx_gpio;
    uint8_t rx_gpio;
} uart_manager_t;

typedef uart_manager_t* uart_manager_handle_t;

void start_uart(uart_manager_handle_t umh);
void init_um(uart_manager_handle_t umh);

#endif