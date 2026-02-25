#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_sntp.h"
#include "containers.h"
#include "joystick_reader.h"

#define WIFI_CONNECTED_TO_AP_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define WIFI_READY_BIT BIT2
#define WIFI_SWITCH_TO_STA_BIT JOYSTICK_X0_BIT
#define WIFI_SWITCH_TO_AP_BIT JOYSTICK_X4095_BIT
#define WIFI_PAUSED_COTROL_BIT JOYSTICK_SW_PRESSED_BIT

#define WIFI_SNTP_WAS_SET_BIT BIT0

typedef struct {
    EventGroupHandle_t wifi_event_group;
    EventGroupHandle_t wifi_led_eg;
    EventGroupHandle_t joy_wifi_eg;
    esp_netif_t* sta_netif;
    esp_netif_t* ap_netif;
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    wifi_config_t ap_config;
    wifi_config_t sta_config;
    dynamic_array_t bit_cbs_arr;
    uint32_t cb_bits;

    // info about time synchronization and control pause
    uint32_t info;
} wifi_manager_handle_t;

void start_nvs_flash();

// configures wifi manager and set its event groups to catch events from joy reader and throw events to led manager
void configure_wifi(wifi_manager_handle_t* wm, EventGroupHandle_t wifi_led_eg, EventGroupHandle_t joy_wifi_eg);

// creates task that is tracking wifi events
void start_wifi_sta(wifi_manager_handle_t* wm);

// wm is wifi manager

void wm_add_event(uint32_t bit, void(*cb)(void*), wifi_manager_handle_t* wifi_manager_handle);
void wm_rm_event(uint32_t bit, wifi_manager_handle_t* wifi_manager_handle);
void wm_switch_to_ap(wifi_manager_handle_t* wm);
void wm_switch_to_sta(wifi_manager_handle_t* wm);

#endif