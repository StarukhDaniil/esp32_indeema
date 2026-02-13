#include <stdio.h>
#include <esp_random.h>
#include <esp_rom_sys.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "led_manager.h"

#define CLICK_PIN 4

void app_main(void)
{
    led_strip_handle_t strip = configure_led();
    bool led_on = false;

    while(1) {
        if (led_on) {
            ESP_ERROR_CHECK(led_strip_set_pixel(strip, 0, 0, BLINK_BRIGHTNESS, 0));
            ESP_ERROR_CHECK(led_strip_refresh(strip));
        }
        else {
            ESP_ERROR_CHECK(led_strip_clear(strip));
        }

        led_on = !led_on;
        vTaskDelay(pdMS_TO_TICKS(BLINK_PERIOD_MS));
    }
}
