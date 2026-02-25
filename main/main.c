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

static const char* TAG = "MAIN";

static wifi_manager_handle_t wm;

static EventGroupHandle_t joy_eg;
static EventGroupHandle_t wifi_led_eg;

static joystick_reader_handle_t joy_reader;
static led_manager_handle_t led_manager;
// static snake_handle_t snake;

static uart_manager_t um;
static uart_manager_handle_t umh = &um;

void app_main(void)
{
    start_nvs_flash();

    joy_eg = xEventGroupCreate();
    wifi_led_eg= xEventGroupCreate();

    configure_wifi(&wm, wifi_led_eg, joy_eg);
    configure_joystick(&joy_reader, joy_eg);
    // configure_led(&led_manager, wifi_led_eg);
    // init_snake(&snake, joy_eg);
    init_um(umh);

    mh_configure_joy_cbs(&joy_reader);
    // mh_configure_wifi_led_cbs(&led_manager);
    // mh_configure_wifi_cbs(&wm);

    // start_wifi_sta(&wm);
    start_joy_event_loop(&joy_reader);
    // start_led_event_loop(&led_manager);
    // snake_start_game(&snake);
    start_uart(umh);
}