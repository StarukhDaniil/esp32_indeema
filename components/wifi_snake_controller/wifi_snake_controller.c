#include "wifi_snake_controller.h"
#include "joystick_reader.h"

#include "esp_log.h"

#define WIFI_PAUSED 0x01
#define SNAKE_PAUSED 0x02

static const char* TAG = "WIFI_SNAKE_CONTROLLER";

static esp_err_t pause_snake(ws_manager_handle_t wsm) {
    xEventGroupSetBits(wsm->eg, SNAKE_PAUSE_BIT);
    EventBits_t bits = xEventGroupWaitBits(
        wsm->eg,
        SNAKE_RECEIVED_PAUSE_BIT,
        pdTRUE,
        pdTRUE, 
        pdMS_TO_TICKS(2000)
    );
        
    if (bits != SNAKE_RECEIVED_PAUSE_BIT) {
        ESP_LOGW(TAG, "Snake did not receive pause bit! Wifi manager is not resumed");
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

static esp_err_t pause_wifi(ws_manager_handle_t wsm) {
    xEventGroupSetBits(wsm->eg, WIFI_PAUSE_BIT);
    EventBits_t bits = xEventGroupWaitBits(
        wsm->eg,
        WIFI_RECEIVED_PAUSE_BIT,
        pdTRUE,
        pdTRUE, 
        pdMS_TO_TICKS(2000)
    );
    
    if (bits != WIFI_RECEIVED_PAUSE_BIT) {
        ESP_LOGW(TAG, "Wifi did not receive pause bit! Snake is not resumed");
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

static esp_err_t resume_snake(ws_manager_handle_t wsm) {
    xEventGroupSetBits(wsm->eg, SNAKE_RESUME_BIT);
    EventBits_t bits = xEventGroupWaitBits(
        wsm->eg,
        SNAKE_RECEIVED_RESUME_BIT,
        pdTRUE,
        pdTRUE, 
        pdMS_TO_TICKS(2000)
    );
    if (bits != SNAKE_RECEIVED_RESUME_BIT) {
        ESP_LOGE(TAG, "Wifi did not receive resume bit! Everyone is stopped!");
        return ESP_ERR_TIMEOUT;
    }

    wsm->who_paused = WIFI_PAUSED;
    return ESP_OK;
}

static esp_err_t resume_wifi(ws_manager_handle_t wsm) {
    xEventGroupSetBits(wsm->eg, WIFI_RESUME_BIT);
    EventBits_t bits = xEventGroupWaitBits(
        wsm->eg,
        WIFI_RECEIVED_RESUME_BIT,
        pdTRUE,
        pdTRUE, 
        pdMS_TO_TICKS(2000)
    );
    if (bits != WIFI_RECEIVED_RESUME_BIT) {
        ESP_LOGE(TAG, "Snake did not receive resume bit! Everyone is stopped!");
        return ESP_ERR_TIMEOUT;
    }

    wsm->who_paused = SNAKE_PAUSED;
    return ESP_OK;
}

static esp_err_t pause_peripheral(ws_manager_handle_t wsm) {
    switch (wsm->who_paused)
    {
    case WIFI_PAUSED:
        if (pause_snake(wsm) == ESP_OK) {
            resume_wifi(wsm);
        }
        else {
            return ESP_ERR_TIMEOUT;
        }
        wsm->who_paused = SNAKE_PAUSED;
        break;
    case SNAKE_PAUSED:
        if (pause_wifi(wsm) == ESP_OK) {
            resume_snake(wsm);
        }
        else {
            return ESP_ERR_TIMEOUT;
        }
        wsm->who_paused = WIFI_PAUSED;
        break;
    }
    return ESP_OK;
}

static esp_err_t first_pause(ws_manager_handle_t wsm) {
    switch (wsm->who_paused)
    {
    case SNAKE_PAUSED:
        if (pause_snake(wsm) != ESP_OK) {
            return ESP_ERR_TIMEOUT;
        }
        break;
    case WIFI_PAUSED:
        if (pause_wifi(wsm) == ESP_OK) {
            return ESP_ERR_TIMEOUT;
        }
        break;
    }
    return ESP_OK;
}

static void event_loop(void* pvParameters) {
    ws_manager_handle_t wsm = pvParameters;
    EventBits_t bits = 0;

    first_pause(wsm);
    for ( ;; ) {
        bits = xEventGroupWaitBits(
            wsm->eg,
            JOYSTICK_SW_PRESSED_BIT,
            pdTRUE,
            pdTRUE,
            portMAX_DELAY
        );
        if (bits != JOYSTICK_SW_PRESSED_BIT) {
            continue;
        }
        pause_peripheral(wsm);
    }
}

void init_wifi_snake_controller(ws_manager_handle_t wsm, EventGroupHandle_t eg) {
    wsm->who_paused = WIFI_PAUSED;
    wsm->eg = eg;
}

void start_controlling_pause(ws_manager_handle_t wsm) {
    xTaskCreate(
        event_loop,
        "WIFI_SNAKE_CONTROLLER",
        1024,
        wsm,
        4,
        &(wsm->task)
    );
}