idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES 
                        "led_manager"
                        "driver"
                        "joystick_reader"
                        "wifi_manager"
                        "main_helper"
                        "snake_game"
                        "uart_manager"
                        "i2c_manager"
                        "wifi_snake_controller")