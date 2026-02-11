#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void vTaskCode(void *pvParameters) {
    printf("Custom task started\n");
    uint32_t counter = 0;
    for ( ;; ) {
        printf("Counter: %lu\n", counter);
        ++counter;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void app_main(void)
{
    TaskHandle_t xHandle = NULL;

    xTaskCreate(
        vTaskCode,
        "IDLE_TASK",
        2048,
        NULL,
        tskIDLE_PRIORITY,
        &xHandle
    );

    uint32_t counter = 0;
    for ( ;; ) {
        printf("Counter: %lu\n", counter);
        ++counter;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
