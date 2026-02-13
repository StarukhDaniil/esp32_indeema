idf_component_register(SRCS "joystick_reader.c"
                    INCLUDE_DIRS "include"
                    REQUIRES 
                        "button"
                        "esp_adc")
