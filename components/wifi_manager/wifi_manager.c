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

#include "wifi_manager.h"
#include "led_manager.h"

#define WIFI_EG_DELAY pdMS_TO_TICKS(1)
#define USER_EG_DELAY pdMS_TO_TICKS(50)

// static EventGroupHandle_t s_wifi_event_group;

static wifi_config_t s_ap_config = {
    .ap = {
        .ssid = "Daniil_ESP32_AP",
        .password = "12345678",
        .max_connection = 5,
        .authmode = WIFI_AUTH_WPA2_PSK,
    },
};
static wifi_config_t s_sta_config = {
    .sta = {
        .ssid = "iPhone-Daniil",
        .password = "12349876",
        .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        .scan_method = WIFI_ALL_CHANNEL_SCAN,
        .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
    },
};

static const char* TAG = "WIFI_MANAGER";

static void wifi_event_loop(void* pvParameters);
static void handle_events();
static void synchronize_time();

void start_nvs_flash(wifi_manager_handle_t wm) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

// handler for only wifi events
static void event_handler(void* arg,
                          esp_event_base_t event_base,
                          int32_t event_id,
                          void* event_data) {
    wifi_manager_handle_t wm = arg;
    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_STA_START) {
            xEventGroupSetBits(wm->wifi_event_group, WIFI_READY_BIT);
        }
        else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            xEventGroupSetBits(wm->wifi_event_group, WIFI_FAIL_BIT);
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(wm->wifi_event_group, WIFI_CONNECTED_TO_AP_BIT);
    }
}

void configure_wifi(wifi_manager_handle_t wm, EventGroupHandle_t wifi_led_eg, EventGroupHandle_t joy_wifi_eg) {
    wm->wifi_led_eg = wifi_led_eg;
    wm->joy_wifi_eg = joy_wifi_eg;
    da_create_array(&(wm->bit_cbs_arr), sizeof(bit_cb_pair_t));
    wm->wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wm->sta_netif = esp_netif_create_default_wifi_sta();
    wm->ap_netif = esp_netif_create_default_wifi_ap();

    wifi_init_config_t wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &event_handler,
        wm,
        &(wm->instance_any_id)));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &event_handler,
        wm,
        &(wm->instance_got_ip)));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &s_sta_config));
}

void start_wifi_sta(wifi_manager_handle_t wm) {
    xTaskCreate(wifi_event_loop,
        "WIFI_Manager",
        4096,
        wm,
        3,
        NULL);
        
    ESP_ERROR_CHECK(esp_wifi_start());
}

void wm_switch_to_ap(wifi_manager_handle_t wm) {
    wm->info &= ~ WIFI_SNTP_WAS_SET_BIT;
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &s_ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "Switched to AP mode!");
    xEventGroupSetBits(wm->wifi_led_eg, LED_WIFI_AP_MODE_BIT);
}

void wm_switch_to_sta(wifi_manager_handle_t wm) {
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &s_sta_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "Switched to STA mode!");
}

static void wifi_event_loop(void* pvParameters) {
    wifi_manager_handle_t wm = pvParameters;
    for ( ;; ) {
        handle_events(wm);
    }
}

// handles events
static void handle_events(wifi_manager_handle_t wm) {
    EventBits_t user_eg_bits = xEventGroupWaitBits(wm->joy_wifi_eg,
        wm->cb_bits,
        pdTRUE,
        pdFALSE,
        USER_EG_DELAY
    );
    
    if (user_eg_bits) {
        ESP_LOGI(TAG, "caught user eg bits: %i", user_eg_bits);
    }

    for(size_t i = 0; i < wm->bit_cbs_arr.curr_size; ++i) {
        if (user_eg_bits & GET_BIT_FROM_HANDLE_ARR(wm, i)) {
            CALL_CB_FROM_HANDLE_ARR(wm, i);
        }
    }

    EventBits_t wifi_eg_bits = xEventGroupWaitBits(wm->wifi_event_group,
        WIFI_CONNECTED_TO_AP_BIT | WIFI_FAIL_BIT | WIFI_READY_BIT | WIFI_SWITCH_TO_STA_BIT | WIFI_SWITCH_TO_AP_BIT,
        pdTRUE,
        pdFALSE,
        WIFI_EG_DELAY);
    
    if (wifi_eg_bits & WIFI_READY_BIT) {
        ESP_LOGI(TAG, "Trying to connect...");
        esp_wifi_connect();
    }
    else if (wifi_eg_bits & WIFI_FAIL_BIT) {
        xEventGroupSetBits(wm->wifi_led_eg, LED_WIFI_FAIL_BIT);
        ESP_LOGI(TAG, "Disconnected, trying to reconnect...");
        esp_wifi_connect();
    }
    else if (wifi_eg_bits & WIFI_CONNECTED_TO_AP_BIT) {
        esp_netif_ip_info_t ip_info;

        if (esp_netif_get_ip_info(wm->sta_netif, &ip_info) == ESP_OK) {
            char ip_str[16];
            esp_ip4addr_ntoa(&ip_info.ip, ip_str, sizeof(ip_str));
            ESP_LOGI(TAG, "Connected! IP: %s", ip_str);
        }
        else {
            ESP_LOGI(TAG, "Connected...");
        }

        xEventGroupSetBits(wm->wifi_led_eg, LED_WIFI_STA_MODE_BIT);

        synchronize_time(wm);
    }
}

static void setup_sntp(wifi_manager_handle_t wm) {
    ESP_LOGI(TAG, "Initializing SNTP...");
    
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_setservername(1, "time.google.com");
    esp_sntp_init();

    wm->info |= WIFI_SNTP_WAS_SET_BIT;
}

static void wait_for_timesync(wifi_manager_handle_t wm) {
    int retry = 0;
    const int rentry_count = 10;
    while((sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && retry < rentry_count)
          && (retry < rentry_count)) {
        ESP_LOGI(TAG, "Waiting for synchronization...");
        vTaskDelay(pdMS_TO_TICKS(2000));
        ++retry;
    }

    if (retry != rentry_count) {
        ESP_LOGI(TAG, "Time synchronized successfully!");
    }
    else {
        ESP_LOGW(TAG, "Was unable to synchronize time");
    }
}

static void synchronize_time(wifi_manager_handle_t wm) {
    ESP_LOGI(TAG, "Trying to synchronize time...");

    if (!(wm->info & WIFI_SNTP_WAS_SET_BIT)) {
        setup_sntp(wm);
    }

    setenv("TZ", "EET-2EEST,M3.5.0/3,M10.5.0/4", 1);
    tzset();

    wait_for_timesync(wm);

    time_t now;
    struct tm time_info;
    time(&now);
    localtime_r(&now, &time_info);

    char strtime_buf[64];
    strftime(strtime_buf, sizeof(strtime_buf), "%c", &time_info);
    ESP_LOGI(TAG, "Current time: %s", strtime_buf);
}

void wm_add_event(uint32_t bit, void(*cb)(void*), wifi_manager_handle_t wifi_manager_handle) {
    wifi_manager_handle->cb_bits |= bit;
    bit_cb_pair_t new_bit_cb_pair = {
        .bit = bit,
        .cb = cb,
    };
    da_push_back(&(wifi_manager_handle->bit_cbs_arr), &new_bit_cb_pair);
}

void wm_rm_event(uint32_t bit, wifi_manager_handle_t wifi_manager_handle) {
    wifi_manager_handle->cb_bits &= ~bit;
    for (size_t i = 0; i < wifi_manager_handle->bit_cbs_arr.curr_size; ++i) {
        if (bit == GET_BIT_FROM_HANDLE_ARR(wifi_manager_handle, i)) {
            da_erase_item(&(wifi_manager_handle->bit_cbs_arr), i);
        }
    }
}