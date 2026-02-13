#ifndef JOYSTICK_READER_H
#define JOYSTICK_READER_H

#define JOYSTICK_SW_GPIO 4
#define JOYSTICK_X_GPIO 6
#define JOYSTICK_Y_GPIO 5

#define RED_FROM_JOY(x, brightness) (uint32_t)((float)(get_r_from_joy_x(x)) * (brightness))
#define GREEN_FROM_JOY(x, brightness) (uint32_t)((float)(get_g_from_joy_x(x)) * (brightness))
#define BLUE_FROM_JOY(x, brightness) (uint32_t)((float)(get_b_from_joy_x(x)) * (brightness))

void configure_joystick(void);

void configure_button_cbs(
    void(*single_clk_cb)(void *arg,void *usr_data),
    void(*double_clk_cb)(void *arg,void *usr_data),
    void(*press_cb)(void *arg,void *usr_data),
    void(*long_press_cb)(void *arg,void *usr_data));

void read_joystick(int* x, int* y, bool* sw_pressed);

uint32_t get_r_from_joy_x(int x);

uint32_t get_g_from_joy_x(int x);

uint32_t get_b_from_joy_x(int x);

float get_bts_from_joy_y(int y);

void sample_single_clk_cb(void *arg,void *usr_data);

void sample_double_clk_cb(void *arg,void *usr_data);

void sample_button_pressed_cb(void *arg,void *usr_data);

void sample_button_long_pressed_cb(void *arg,void *usr_data);

#endif