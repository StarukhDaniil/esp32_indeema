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

static const char* TAG = "MAIN";

void app_main(void)
{
    start_wifi_sta();
}