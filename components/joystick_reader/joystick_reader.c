#include <stdio.h>
#include <stdbool.h>
#include "joystick_reader.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "button_gpio.h"
#include "iot_button.h"
#include "esp_event.h"

static const char* TAG = "JOYSTICK_READER";

static void init_adc(joystick_reader_handle_t joy_reader) {
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &(joy_reader->adc)));
}

static void configure_adc_pin(joystick_reader_handle_t joy_reader, int gpio, adc_channel_t* out_channel) {
    adc_unit_t unit = ADC_UNIT_1;
    adc_channel_t channel = ADC_CHANNEL_0;

    ESP_ERROR_CHECK(adc_oneshot_io_to_channel(gpio, &unit, &channel));
    if (unit != ADC_UNIT_1) {
        ESP_LOGE(TAG, "GPIO %d mapped to ADC%d. Use ADC1 pins for joystick.", gpio, unit + 1);
        abort();
    }

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel(joy_reader->adc, channel, &chan_cfg));

    *out_channel = channel;
}

static void configure_button(joystick_reader_handle_t joy_reader) {
    button_config_t btn_cfg = {0};
    button_gpio_config_t btn_gpio_cfg = {
        .gpio_num = JOYSTICK_SW_GPIO,
        .active_level = 0,
    };

    ESP_ERROR_CHECK(iot_button_new_gpio_device(&btn_cfg, &btn_gpio_cfg, &(joy_reader->btn)));
}

void configure_button_cbs(
    joystick_reader_handle_t joy_reader,
    void(*single_clk_cb)(void *arg,void *usr_data),
    void(*double_clk_cb)(void *arg,void *usr_data),
    void(*press_cb)(void *arg,void *usr_data),
    void(*long_press_cb)(void *arg,void *usr_data))
{
    iot_button_register_cb(joy_reader->btn, BUTTON_SINGLE_CLICK, NULL, single_clk_cb, NULL);
    iot_button_register_cb(joy_reader->btn, BUTTON_DOUBLE_CLICK, NULL, double_clk_cb, NULL);
    iot_button_register_cb(joy_reader->btn, BUTTON_PRESS_DOWN, NULL, press_cb, NULL);
    iot_button_register_cb(joy_reader->btn, BUTTON_LONG_PRESS_START, NULL, long_press_cb, NULL);
}

void configure_joy_x0_cbs(joystick_reader_handle_t joy_reader, void(*x0_cb)(void*)) {
    joy_reader->x0_cb = x0_cb;
}

void configure_joy_x4095_cbs(joystick_reader_handle_t joy_reader, void(*x4095_cb)(void*)) {
    joy_reader->x4095_cb = x4095_cb;
}

void configure_joy_y0_cbs(joystick_reader_handle_t joy_reader, void(*y0_cb)(void*)) {
    joy_reader->y0_cb = y0_cb;
}

void configure_joy_y4095_cbs(joystick_reader_handle_t joy_reader, void(*y4095_cb)(void*)) {
    joy_reader->y4095_cb = y4095_cb;
}

void configure_joy_sw_pressed_cbs(joystick_reader_handle_t joy_reader, void(*sw_pressed_cb)(void*)) {
    joy_reader->sw_pressed = sw_pressed_cb;
}

void configure_joystick(joystick_reader_handle_t joy_reader, EventGroupHandle_t eg) {
    assert(eg != NULL);
    joy_reader->event_group = eg;

    init_adc(joy_reader);

    configure_adc_pin(joy_reader, JOYSTICK_X_GPIO, &(joy_reader->chan_x));
    configure_adc_pin(joy_reader, JOYSTICK_Y_GPIO, &(joy_reader->chan_y));
    configure_button(joy_reader);

    gpio_config_t sw_cfg = {
        .pin_bit_mask = 1ULL << JOYSTICK_SW_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    ESP_ERROR_CHECK(gpio_config(&sw_cfg));
    ESP_LOGI(TAG, "joy is configured");
}

void read_joystick(joystick_reader_handle_t joy_reader, int* x, int* y, bool* sw_pressed) {
    ESP_ERROR_CHECK(adc_oneshot_read(joy_reader->adc, joy_reader->chan_x, x));
    ESP_ERROR_CHECK(adc_oneshot_read(joy_reader->adc, joy_reader->chan_y, y));
    *sw_pressed = (gpio_get_level(JOYSTICK_SW_GPIO) == 0);
}

uint32_t get_r_from_joy_x(int x) {
    if (x < 1500) {
        return 255;
    }
    return 0;
}

uint32_t get_g_from_joy_x(int x) {
    if (x < 2300 && x > 1499) {
        return 255;
    }
    return 0;
}

uint32_t get_b_from_joy_x(int x) {
    if (x > 2299) {
        return 255;
    }
    return 0;
}

float get_bts_from_joy_y(int y) {
    if (y == 0) {       // to prevent floating point precision error
        return 0.0f;
    }
    return (float)(y) / 4095.0f;
}

void sample_single_clk_cb(void *arg,void *usr_data) {
    ESP_LOGI(TAG, "single click");
}

void sample_double_clk_cb(void *arg,void *usr_data) {
    ESP_LOGI(TAG, "double click");
}

void sample_button_pressed_cb(void *arg,void *usr_data) {
    ESP_LOGI(TAG, "button pressed");
}

void sample_button_long_pressed_cb(void *arg,void *usr_data) {
    ESP_LOGI(TAG, "button long pressed");
}

void joy_event_loop(void* pvParameters) {
    joystick_reader_handle_t joy_reader = pvParameters;
    int x;
    int y;
    bool sw_pressed;

    ESP_LOGI(TAG, "Starting joy event loop...");

    for ( ;; ) {
        read_joystick(joy_reader, &x, &y, &sw_pressed);

        if ((x == 0) && (joy_reader->events != JOYSTICK_X0_BIT)) {
            joy_reader->events = JOYSTICK_X0_BIT;
            if (joy_reader->x0_cb) {
                joy_reader->x0_cb(joy_reader);
            }
        }
        else if ((x == 4095) && (joy_reader->events != JOYSTICK_X4095_BIT)) {
            joy_reader->events = JOYSTICK_X4095_BIT;
            if (joy_reader->x4095_cb) {
                joy_reader->x4095_cb(joy_reader);
            }
        }
        else if ((y == 0) && (joy_reader->events != JOYSTICK_Y0_BIT)) {
            joy_reader->events = JOYSTICK_Y0_BIT;
            if (joy_reader->y0_cb) {
                joy_reader->y0_cb(joy_reader);
            }
        }
        else if ((y == 4095) && (joy_reader->events != JOYSTICK_Y4095_BIT)) {
            joy_reader->events = JOYSTICK_Y4095_BIT;
            if (joy_reader->y4095_cb) {
                joy_reader->y4095_cb(joy_reader);
            }
        }
        else if (sw_pressed && (joy_reader->events != JOYSTICK_SW_PRESSED_BIT)) {
            joy_reader->events = JOYSTICK_SW_PRESSED_BIT;
            if (joy_reader->sw_pressed) {
                joy_reader->sw_pressed(joy_reader);
            }
        }
        else if ((x != 0) && (x != 4095) && (y != 0) && (y != 4095) && !sw_pressed) {
            joy_reader->events = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(JOY_LOOP_SLEEP_MS));
    }
}

BaseType_t start_joy_event_loop(joystick_reader_handle_t joy_reader) {
    return xTaskCreate(joy_event_loop,
                "LED_MANAGER",
                2048,
                joy_reader,
                3,
                &(joy_reader->joy_task));
}