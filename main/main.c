#include <stdio.h>
#include <esp_random.h>
#include <esp_rom_sys.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "led_manager.h"
#include "joystick_reader.h"
#include "stdbool.h"

static const char* TAG = "MAIN";

void app_main(void)
{
    led_strip_handle_t strip = configure_led();
    bool led_on = false;

    int joy_x_val = 0;
    int joy_y_val = 0;
    bool joy_sw_pressed = false;

    float brightness = 0.0f;

    configure_joystick();

    while(1) {
        read_joystick(&joy_x_val, &joy_y_val, &joy_sw_pressed);

        brightness = get_bts_from_joy_y(joy_y_val);

        ESP_ERROR_CHECK(led_strip_set_pixel(strip, 0,
            RED_FROM_JOY(joy_x_val, brightness),
            GREEN_FROM_JOY(joy_x_val, brightness),
            BLUE_FROM_JOY(joy_x_val, brightness)));
        ESP_ERROR_CHECK(led_strip_refresh(strip));
            
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
