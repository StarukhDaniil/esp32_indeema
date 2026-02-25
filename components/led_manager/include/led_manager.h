#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include "led_strip.h"
#include "esp_log.h"
#include "containers.h"
#include "wifi_manager.h"

#define LED_WIFI_FAIL_BIT BIT0
#define LED_WIFI_STA_MODE_BIT BIT1
#define LED_WIFI_AP_MODE_BIT BIT2

#define MATRIX_CURRPX_IDX 0
#define MATRIX_UPDATE_BIT BIT0
#define MATRIX_UPDATE_PERIOD 500
#define MATRIX_LED_COUNT 64

typedef struct {
    TaskHandle_t led_task;
    led_strip_handle_t strip;
    EventGroupHandle_t event_group;
    // array of bit-callback pairs
    dynamic_array_t bit_cbs_arr;
    // variable that stores all bits that have to be tracked
    uint32_t cb_bits;
} led_manager_handle_t;

void configure_matrix(led_manager_handle_t* matrix, EventGroupHandle_t eg);
void configure_led(led_manager_handle_t* led_manager_handle, EventGroupHandle_t eg);
BaseType_t start_led_event_loop(led_manager_handle_t* led_manager_handle);
void led_add_event(uint32_t bit, void(*cb)(void*), led_manager_handle_t* led_manager_handle);
void led_rm_event(uint32_t bit, led_manager_handle_t* led_manager_handle);

#endif