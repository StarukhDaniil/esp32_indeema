#include "uart_manager.h"
#include "esp_log.h"
#include "memory.h"

#define DEFAULT_RX_BUFF_SIZE 1024
#define DEFAULT_UART_PORT UART_NUM_1
#define DEFAULT_TX_GPIO 3
#define DEFAULT_RX_GPIO 8

void init_um(uart_manager_handle_t umh)
{
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    *umh = (uart_manager_t) {
        .port = DEFAULT_UART_PORT,
        .rx_buf_size = DEFAULT_RX_BUFF_SIZE,
        .tx_gpio = DEFAULT_TX_GPIO,
        .rx_gpio = DEFAULT_RX_GPIO,
    };
    
    uart_driver_install(umh->port, umh->rx_buf_size * 2, 0, 0, NULL, 0);
    uart_param_config(umh->port, &uart_config);
    uart_set_pin(umh->port, umh->tx_gpio, umh->rx_gpio, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

int sendData(const char* logName, const char* data)
{
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(UART_NUM_1, data, len);
    ESP_LOGI(logName, "Wrote %d bytes", txBytes);
    return txBytes;
}

static void tx_task(void *arg)
{
    static const char *TX_TASK_TAG = "TX_TASK";
    while (1) {
        sendData(TX_TASK_TAG, "{ \"id\": 103 }\n");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

static void rx_task(void *arg)
{
    uart_manager_handle_t umh = arg;
    static const char *RX_TASK_TAG = "RX_TASK";
    uint8_t* data = (uint8_t*) malloc(umh->rx_buf_size + 1);
    while (1) {
        const int rxBytes = uart_read_bytes(UART_NUM_1, data, umh->rx_buf_size, 1000 / portTICK_PERIOD_MS);
        if (rxBytes > 0) {
            data[rxBytes] = 0;
            ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
        }
    }
    free(data);
}

void start_uart(uart_manager_handle_t umh) {
    xTaskCreate(rx_task, "uart_rx_task", 8192, umh, configMAX_PRIORITIES - 1, NULL);
    xTaskCreate(tx_task, "uart_tx_task", 8192, umh, configMAX_PRIORITIES - 2, NULL);
}