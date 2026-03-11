idf_component_register(SRCS "wifi_snake_controller.c"
                    INCLUDE_DIRS "include"
                    REQUIRES
                        "snake_game"
                        "wifi_manager"
                        "joystick_reader")