#include "mqtt_manager.h"
#include "esp_system.h"
#include "esp_log.h"
#include "mqtt5_client.h"

#define BROKER_URI "mqtt://broker.hivemq.com"
#define TOPIC "esp32mqtt/daniil_esp32"
#define TOPIC_SUBSCRIBE_ID 1

static const char *TAG = "MQTT_MANAGER";

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt5_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    mqtt_manager_handle_t mmh = handler_args;
    int msg_id;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Connected to MQTT broker!");
        // esp_mqtt5_subscribe_property_config_t subscribe_property = {
        //     .is_share_subscribe = 0,
        //     .no_local_flag = 1,
        //     .retain_handle = 1,
        //     .retain_as_published_flag = 1,
        //     .subscribe_id = TOPIC_SUBSCRIBE_ID,
        // };

        esp_mqtt5_subscribe_property_config_t sub_props = {
            .no_local_flag = true,
            .retain_handle = 1,
            .retain_as_published_flag = false,
        };

        esp_mqtt5_client_set_subscribe_property(mmh->client, &sub_props);
        int msg_id = esp_mqtt_client_subscribe(client, TOPIC, 0);

        if (msg_id == -1) {
            ESP_LOGE(TAG, "Failed to send subscribe request");
        }
        else {
            ESP_LOGI(TAG, "Sent subscribe successful, msg_id=%d", msg_id);
        }
        ESP_LOGI(TAG, "Sent subscribe");
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "Subscribed!");
        char msg[] = "subscribed";
        msg_id = esp_mqtt_client_publish(mmh->client, TOPIC, msg, strlen(msg), 0, 1);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "Unsubsribed");
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

inline static void send_status(mqtt_manager_handle_t mmh) {
    char msg[] = "status";
    esp_mqtt_client_publish(mmh->client, TOPIC, msg, strlen(msg), 0, 0);
}

static void event_loop(void* pvParameters) {
    mqtt_manager_handle_t mmh = pvParameters;
    EventBits_t bits = 0;

    for ( ;; ) {
        bits = xEventGroupWaitBits(
            mmh->eg,
            MQTT_MANAGER_SEND_STATUS_BIT,
            pdTRUE,
            pdFALSE,
            portMAX_DELAY
        );

        if (bits & MQTT_MANAGER_SEND_STATUS_BIT) {
            send_status(mmh);
        }
    }
}

BaseType_t mqtt_manager_start_loop(mqtt_manager_handle_t mmh) {
    return xTaskCreate(
        event_loop,
        "MQTT_MANAGER",
        8192,
        mmh,
        5,
        &(mmh->task)
    );
}

void init_mqtt_manager(mqtt_manager_handle_t mmh, EventGroupHandle_t eg) {
    char last_will_msg[] = "I turned off, this is my last will";
    mmh->eg = eg;

    esp_mqtt_client_config_t mqtt5_cfg = {
        .broker.address.uri = BROKER_URI,
        .broker.address.port = 1883,
        .session.protocol_ver = MQTT_PROTOCOL_V_5,
        .network.disable_auto_reconnect = false,
        .credentials = {
            .client_id = "daniil_esp32_id",
            .username = "daniil_esp32",
        },
        .session.last_will.msg = last_will_msg,
        .session.last_will.qos = 0,
        .session.last_will.msg_len = strlen(last_will_msg),
        .session.last_will.retain = 1,
        .session.last_will.topic = "daniil_esp32/last_will",
    };

    esp_mqtt5_connection_property_config_t connection_cfg = {
        .session_expiry_interval = 60,
        .request_resp_info = 1,
        .maximum_packet_size = 1024,
        .receive_maximum = 65535,
        .request_problem_info = 1,
        .will_delay_interval = 10,        
    };
    
    mmh->client = esp_mqtt_client_init(&mqtt5_cfg);

    esp_mqtt5_client_set_connect_property(mmh->client, &connection_cfg);

    ESP_ERROR_CHECK(esp_mqtt_client_register_event(mmh->client, ESP_EVENT_ANY_ID, mqtt5_event_handler, mmh));
    ESP_ERROR_CHECK(esp_mqtt_client_start(mmh->client));
}