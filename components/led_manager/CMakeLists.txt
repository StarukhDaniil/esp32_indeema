idf_component_register(
    SRCS "led_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES
        "led_strip"
        "cpu_loader"
        "esp_event"
        "containers"
        "wifi_manager"
)