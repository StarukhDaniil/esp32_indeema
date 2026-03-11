#ifndef WIFI_SNAKE_CONTROLLER_H
#define WIFI_SNAKE_CONTROLLER_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"

#define WIFI_PAUSE_BIT BIT6
#define SNAKE_PAUSE_BIT BIT7

#define WIFI_RECEIVED_PAUSE_BIT BIT8
#define SNAKE_RECEIVED_PAUSE_BIT BIT9

#define WIFI_RESUME_BIT BIT10
#define SNAKE_RESUME_BIT BIT11

#define WIFI_RECEIVED_RESUME_BIT BIT12
#define SNAKE_RECEIVED_RESUME_BIT BIT13

typedef struct {
    TaskHandle_t task;
    EventGroupHandle_t eg;
    uint8_t who_paused;
} ws_manager_t;

typedef ws_manager_t* ws_manager_handle_t;

void init_wifi_snake_controller(ws_manager_handle_t wsm, EventGroupHandle_t eg);

// tracks when sw is pressed and pauses wifi or snake
void start_controlling_pause(ws_manager_handle_t wsm);

#endif // WIFI_SNAKE_CONTROLLER_H