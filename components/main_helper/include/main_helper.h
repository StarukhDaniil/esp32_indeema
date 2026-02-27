#ifndef MAIN_HELPER_H
#define MAIN_HELPER_H

#include "wifi_manager.h"
#include "led_manager.h"
#include "joystick_reader.h"

// mh is main helper

void mh_configure_wifi_cbs(wifi_manager_handle_t wm);
void mh_configure_wifi_led_cbs(led_manager_handle_t led_manager);
void mh_configure_joy_cbs(joystick_reader_handle_t joy_reader);

#endif // MAIN_HELPER_H