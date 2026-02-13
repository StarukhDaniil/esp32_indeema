#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include "led_strip.h"
#include "esp_log.h"

#define BLINK_GPIO 48
#define BLINK_PERIOD_MS 500
#define BLINK_LED_COUNT 1
#define BLINK_BRIGHTNESS 20

led_strip_handle_t configure_led(void);

#endif