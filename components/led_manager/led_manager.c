#include "led_strip.h"
#include "esp_log.h"
#include "esp_event.h"
#include "containers.h"

#include "led_manager.h"

#define PARAMS_STRIP_IDX 0
#define PARAMS_EVENT_GROUP_IDX 0

#define BLINK_GPIO 48
#define BLINK_PERIOD_MS 500
#define BLINK_LED_COUNT 1
#define BLINK_BRIGHTNESS 20

static const char* TAG = "LED_MANAGER";

void configure_led(led_manager_handle_t led_manager_handle, EventGroupHandle_t eg) {
    assert(eg != NULL);
    
    led_manager_handle->event_group = eg;
    da_create_array(&(led_manager_handle->bit_cbs_arr), sizeof(bit_cb_pair_t));

    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = BLINK_LED_COUNT,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags = {
            .invert_out = false,
        }
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .mem_block_symbols = 64,
        .flags = {
            .with_dma = false,
        }
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &(led_manager_handle->strip)));
    ESP_ERROR_CHECK(led_strip_clear(led_manager_handle->strip));
    ESP_LOGI(TAG, "WS2812 initialized on GPIO %d", BLINK_GPIO);
}



void led_add_event(uint32_t bit, void(*cb)(void*), led_manager_handle_t led_manager_handle) {
    led_manager_handle->cb_bits |= bit;
    bit_cb_pair_t new_bit_cb_pair = {
        .bit = bit,
        .cb = cb,
    };
    da_push_back(&(led_manager_handle->bit_cbs_arr), &new_bit_cb_pair);
}

void led_rm_event(uint32_t bit, led_manager_handle_t led_manager_handle) {
    led_manager_handle->cb_bits &= ~bit;
    for (size_t i = 0; i < led_manager_handle->bit_cbs_arr.curr_size; ++i) {
        if (bit == GET_BIT_FROM_HANDLE_ARR(led_manager_handle, i)) {
            da_erase_item(&(led_manager_handle->bit_cbs_arr), i);
        }
    }
}

static void handle_led_events(led_manager_handle_t led_manager_handle) {    
    EventBits_t bits = xEventGroupWaitBits(
        led_manager_handle->event_group,
        led_manager_handle->cb_bits,
        pdTRUE,
        pdFALSE,
        portMAX_DELAY);

    ESP_LOGI(TAG, "Led manager caught %lu", bits);
    
    for (size_t i = 0; i < led_manager_handle->bit_cbs_arr.curr_size; ++i) {
        if (bits & GET_BIT_FROM_HANDLE_ARR(led_manager_handle, i)) {
            CALL_CB_FROM_HANDLE_ARR(led_manager_handle, i);
        }
    }
}

static void led_event_loop(void* pvParameters) {
    ESP_LOGI(TAG, "Starting led event loop...");
    led_manager_handle_t led_manager_handle = pvParameters;
    for ( ;; ) {
        handle_led_events(led_manager_handle);
    }
}

BaseType_t start_led_event_loop(led_manager_handle_t led_manager_handle) {
    return xTaskCreate(led_event_loop,
        "LED_MANAGER",
        4096,
        led_manager_handle,
        3,
        &(led_manager_handle->led_task));
}