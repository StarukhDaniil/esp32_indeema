#include <stdio.h>
#include <esp_random.h>
#include <esp_rom_sys.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "stdbool.h"

#include "led_manager.h"
#include "joystick_reader.h"
#include "wifi_manager.h"
#include "main_helper.h"
#include "snake_game.h"
#include "uart_manager.h"
#include "wifi_snake_controller.h"
#include "i2c_manager.h"
#include "spi_manager.h"

// static const char* TAG = "MAIN";

static EventGroupHandle_t joy_eg;
static EventGroupHandle_t wifi_led_eg;

static wifi_manager_t wm;
static joystick_reader_t joy_reader;
static led_manager_t led_manager;
static snake_t snake;
static ws_manager_t wifi_snake_manager;
static i2c_manager_t i2c_manager;
static spi_manager_t spi_manager;


// static uart_manager_t um;

void app_main(void)
{
    start_nvs_flash();

    joy_eg = xEventGroupCreate();
    wifi_led_eg= xEventGroupCreate();

    configure_wifi(&wm, wifi_led_eg, joy_eg);
    configure_joystick(&joy_reader, joy_eg);
    configure_led(&led_manager, wifi_led_eg);
    init_snake(&snake, joy_eg);
    // init_um(&um);
    init_wifi_snake_controller(&wifi_snake_manager, joy_eg);
    init_i2c_manager(&i2c_manager, joy_eg);
    init_spi_manager(&spi_manager, joy_eg);

    vTaskDelay(pdMS_TO_TICKS(500));
    lsm6ds3_init_xl(&spi_manager);

    mh_configure_joy_cbs(&joy_reader);
    mh_configure_wifi_led_cbs(&led_manager);
    mh_configure_wifi_cbs(&wm);

    start_wifi_sta(&wm);
    start_joy_event_loop(&joy_reader);
    start_led_event_loop(&led_manager);
    // start_uart(&um);
    start_controlling_pause(&wifi_snake_manager);
    snake_start_game(&snake);
    i2c_manager_start_event_loop(&i2c_manager);
    spi_manager_start_event_loop(&spi_manager);
}