idf_component_register(SRCS "snake_game.c"
                    INCLUDE_DIRS "include"
                    REQUIRES
                        "led_manager"
                        "containers"
                        "wifi_snake_controller")