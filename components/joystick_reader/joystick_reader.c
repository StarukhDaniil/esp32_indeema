#include <stdio.h>
#include <stdbool.h>
#include "joystick_reader.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char* TAG = "JOYSTICK_READER";

static adc_oneshot_unit_handle_t s_adc;

static adc_channel_t s_chan_x;
static adc_channel_t s_chan_y;

static void init_adc() {
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &s_adc));
}

static void configure_adc_pin(int gpio, adc_channel_t* out_channel) {
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

    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc, channel, &chan_cfg));

    *out_channel = channel;
}

void configure_joystick(void) {
    init_adc();

    configure_adc_pin(JOYSTICK_X_GPIO, &s_chan_x);
    configure_adc_pin(JOYSTICK_Y_GPIO, &s_chan_y);

    gpio_config_t sw_cfg = {
        .pin_bit_mask = 1ULL << JOYSTICK_SW_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    ESP_ERROR_CHECK(gpio_config(&sw_cfg));
}

void read_joystick(int* x, int* y, bool* sw_pressed) {
    ESP_ERROR_CHECK(adc_oneshot_read(s_adc, s_chan_x, x));
    ESP_ERROR_CHECK(adc_oneshot_read(s_adc, s_chan_y, y));
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