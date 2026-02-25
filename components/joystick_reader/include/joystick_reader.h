#ifndef JOYSTICK_READER_H
#define JOYSTICK_READER_H

#include "esp_adc/adc_oneshot.h"
#include "button_gpio.h"
#include "esp_event.h"
#include "freertos/task.h"

#define JOYSTICK_SW_GPIO 4
#define JOYSTICK_X_GPIO 6
#define JOYSTICK_Y_GPIO 5
#define JOY_READ_FREQ 10
#define JOY_LOOP_SLEEP_MS 1000 / JOY_READ_FREQ

#define JOYSTICK_X0_BIT BIT0
#define JOYSTICK_X4095_BIT BIT1
#define JOYSTICK_Y0_BIT BIT2
#define JOYSTICK_Y4095_BIT BIT3
#define JOYSTICK_SW_PRESSED_BIT BIT4

#define RED_FROM_JOY(x, brightness) (uint32_t)((float)(get_r_from_joy_x(x)) * (brightness))
#define GREEN_FROM_JOY(x, brightness) (uint32_t)((float)(get_g_from_joy_x(x)) * (brightness))
#define BLUE_FROM_JOY(x, brightness) (uint32_t)((float)(get_b_from_joy_x(x)) * (brightness))

typedef struct {
    EventGroupHandle_t event_group;
    adc_oneshot_unit_handle_t adc;
    adc_channel_t chan_x;
    adc_channel_t chan_y;
    button_handle_t btn;
    TaskHandle_t joy_task;

    // for tracking events and preventing repeatable events
    // for example, if user holds joy on x=0, joystick_reader doesn`t throw events every 50 milliseconds
    uint32_t events;

    void(*x0_cb)(void*);
    void(*x4095_cb)(void*);
    void(*y0_cb)(void*);
    void(*y4095_cb)(void*);
    void(*sw_pressed)(void*);
} joystick_reader_handle_t;

// setup joy, button and event group
void configure_joystick(joystick_reader_handle_t* joy_reader, EventGroupHandle_t eg);

void configure_button_cbs(
    joystick_reader_handle_t* joy_reader,
    void(*single_clk_cb)(void *arg,void *usr_data),
    void(*double_clk_cb)(void *arg,void *usr_data),
    void(*press_cb)(void *arg,void *usr_data),
    void(*long_press_cb)(void *arg,void *usr_data));

void read_joystick(joystick_reader_handle_t* joy_reader, int* x, int* y, bool* sw_pressed);

// functions for converting joy position to color and brightness on LED
uint32_t get_r_from_joy_x(int x);
uint32_t get_g_from_joy_x(int x);
uint32_t get_b_from_joy_x(int x);
float get_bts_from_joy_y(int y);

void sample_single_clk_cb(void *arg,void *usr_data);
void sample_double_clk_cb(void *arg,void *usr_data);
void sample_button_pressed_cb(void *arg,void *usr_data);
void sample_button_long_pressed_cb(void *arg,void *usr_data);

void configure_joy_x0_cbs(joystick_reader_handle_t* joy_reader, void(*x0_cb)(void*));
void configure_joy_x4095_cbs(joystick_reader_handle_t* joy_reader, void(*x4095_cb)(void*));
void configure_joy_y0_cbs(joystick_reader_handle_t* joy_reader, void(*y0_cb)(void*));
void configure_joy_y4095_cbs(joystick_reader_handle_t* joy_reader, void(*y4095_cb)(void*));
void configure_joy_sw_pressed_cbs(joystick_reader_handle_t* joy_reader, void(*sw_pressed_cb)(void*));

// creates a task that tracks joystick and throws events
BaseType_t start_joy_event_loop(joystick_reader_handle_t* joy_reader);

#endif