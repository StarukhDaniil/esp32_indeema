#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include "wifi_manager.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "esp_task.h"

#define MQTT_MANAGER_SEND_STATUS_BIT BIT0

typedef struct {
    TaskHandle_t task;
    EventGroupHandle_t eg;
    esp_mqtt_client_handle_t client;
} mqtt_manager_t;

typedef mqtt_manager_t* mqtt_manager_handle_t;

BaseType_t mqtt_manager_start_loop(mqtt_manager_handle_t mmh);
void init_mqtt_manager(mqtt_manager_handle_t mmh, EventGroupHandle_t eg);


#endif // MQTT_MANAGER_H