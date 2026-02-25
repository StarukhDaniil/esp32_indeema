#include "main_helper.h"
#include "wifi_manager.h"
#include "led_manager.h"

static void on_wifi_STA_switch_cb(void* led_manager) {
    ESP_LOGI("LED_MANAGER", "switch to STA callback");
    ESP_ERROR_CHECK(led_strip_set_pixel(((led_manager_handle_t*)led_manager)->strip, 0, 0, 255, 0));
    ESP_ERROR_CHECK(led_strip_refresh(((led_manager_handle_t*)led_manager)->strip));
}

static void on_wifi_AP_switch_cb(void* led_manager) {
    ESP_LOGI("LED_MANAGER", "switch to AP callback");
    ESP_ERROR_CHECK(led_strip_set_pixel(((led_manager_handle_t*)led_manager)->strip, 0, 0, 0, 255));
    ESP_ERROR_CHECK(led_strip_refresh(((led_manager_handle_t*)led_manager)->strip));
}

static void on_wifi_fail_cb(void* led_manager) {
    ESP_LOGI("LED_MANAGER", "wifi fail callback");
    ESP_ERROR_CHECK(led_strip_set_pixel(((led_manager_handle_t*)led_manager)->strip, 0, 255, 0, 0));
    ESP_ERROR_CHECK(led_strip_refresh(((led_manager_handle_t*)led_manager)->strip));
}

// defines how led manager will handle events from wifi manager
void mh_configure_wifi_led_cbs(led_manager_handle_t* led_manager) {
    led_add_event(LED_WIFI_STA_MODE_BIT, on_wifi_STA_switch_cb, led_manager);
    led_add_event(LED_WIFI_AP_MODE_BIT, on_wifi_AP_switch_cb, led_manager);
    led_add_event(LED_WIFI_FAIL_BIT, on_wifi_fail_cb, led_manager);
}

static void on_joy_x0_cb(void* joy_reader) {
    ESP_LOGI("JOY_READER", "x0 callback");
    xEventGroupClearBits(((joystick_reader_handle_t*)joy_reader)->event_group, 0x0000ffff);
    xEventGroupSetBits(((joystick_reader_handle_t*)joy_reader)->event_group, JOYSTICK_X0_BIT);
}

static void on_joy_x4095_cb(void* joy_reader) {
    ESP_LOGI("JOY_READER", "x4095 callback");
    xEventGroupClearBits(((joystick_reader_handle_t*)joy_reader)->event_group, 0x0000ffff);
    xEventGroupSetBits(((joystick_reader_handle_t*)joy_reader)->event_group, JOYSTICK_X4095_BIT);
}

static void on_joy_y0_cb(void* joy_reader) {
    ESP_LOGI("JOY_READER", "y0 callback");
    xEventGroupClearBits(((joystick_reader_handle_t*)joy_reader)->event_group, 0x0000ffff);
    xEventGroupSetBits(((joystick_reader_handle_t*)joy_reader)->event_group, JOYSTICK_Y0_BIT);
}

static void on_joy_y4095_cb(void* joy_reader) {
    ESP_LOGI("JOY_READER", "y4095 callback");
    xEventGroupClearBits(((joystick_reader_handle_t*)joy_reader)->event_group, 0x0000ffff);
    xEventGroupSetBits(((joystick_reader_handle_t*)joy_reader)->event_group, JOYSTICK_Y4095_BIT);
}

static void on_joy_sw_pressed_cb(void* joy_reader) {
    ESP_LOGI("JOY_READER", "sw pressed callback");
    xEventGroupClearBits(((joystick_reader_handle_t*)joy_reader)->event_group, 0x0000ffff);
    xEventGroupSetBits(((joystick_reader_handle_t*)joy_reader)->event_group, JOYSTICK_SW_PRESSED_BIT);
}

// configures what events joy reader will throw
void mh_configure_joy_cbs(joystick_reader_handle_t* joy_reader) {
    configure_joy_x0_cbs(joy_reader, on_joy_x0_cb);
    configure_joy_x4095_cbs(joy_reader, on_joy_x4095_cb);
    configure_joy_y0_cbs(joy_reader, on_joy_y0_cb);
    configure_joy_y4095_cbs(joy_reader, on_joy_y4095_cb);
    configure_joy_sw_pressed_cbs(joy_reader, on_joy_sw_pressed_cb);
}

static void mh_wifi_switch_to_sta_wrap(void* pvParameters) {
    wm_switch_to_sta(pvParameters);
}

static void mh_wifi_switch_to_ap_wrap(void* pvParameters) {
    wm_switch_to_ap(pvParameters);
}

// defines how wifi manager will handle events from joy reader 
void mh_configure_wifi_cbs(wifi_manager_handle_t* wm) {
    wm_add_event(WIFI_SWITCH_TO_AP_BIT, mh_wifi_switch_to_ap_wrap, wm);
    wm_add_event(WIFI_SWITCH_TO_STA_BIT, mh_wifi_switch_to_sta_wrap, wm);
}