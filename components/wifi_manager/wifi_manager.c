#include <stdio.h>
#include <string.h>
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

#include "wifi_manager.h"

static EventGroupHandle_t s_wifi_event_group;
static esp_netif_t* s_sta_netif = NULL;
static esp_event_handler_instance_t instance_any_id;
static esp_event_handler_instance_t instance_got_ip;

static const char* TAG = "WIFI_MANAGER";

static void wifi_event_loop(void* pvParameters);
static void wait_for_events();

static void start_nvs_flash() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

static void event_handler(void* arg,
                          esp_event_base_t event_base,
                          int32_t event_id,
                          void* event_data) {
    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_STA_START) {
            xEventGroupSetBits(s_wifi_event_group, WIFI_READY_BIT);
        }
        else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_TO_AP_BIT);
    }
}

void start_wifi_sta() {
    start_nvs_flash();

    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    s_sta_netif =  esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &event_handler,
        NULL,
        &instance_any_id));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &event_handler,
        NULL,
        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "iPhone-Daniil",
            .password = "12349876",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .scan_method = WIFI_ALL_CHANNEL_SCAN,
            .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    
    xTaskCreate(wifi_event_loop,
        "WIFI_Manager",
        4096,
        NULL,
        3,
        NULL);
        
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void wifi_event_loop(void* pvParameters) {
    for ( ;; ) {
        wait_for_events();
    }
}

static void wait_for_events() {
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
        WIFI_CONNECTED_TO_AP_BIT | WIFI_FAIL_BIT | WIFI_READY_BIT,
        pdTRUE,
        pdFALSE,
        portMAX_DELAY);
    
    if (bits & WIFI_READY_BIT) {
        ESP_LOGI(TAG, "Trying to connect...");
        esp_wifi_connect();
    }
    else if (bits & WIFI_FAIL_BIT) {
        vTaskDelay(pdMS_TO_TICKS(2000));
        ESP_LOGI(TAG, "Disconnected, trying to reconnect...");
        esp_wifi_connect();
    }
    else if (bits & WIFI_CONNECTED_TO_AP_BIT) {
        esp_netif_ip_info_t ip_info;

        if (esp_netif_get_ip_info(s_sta_netif, &ip_info) == ESP_OK) {
            char ip_str[16];
            esp_ip4addr_ntoa(&ip_info.ip, ip_str, sizeof(ip_str));
            ESP_LOGI(TAG, "Connected! IP: %s", ip_str);
        }
        else {
            ESP_LOGI(TAG, "Connected...");
        }
    }
}