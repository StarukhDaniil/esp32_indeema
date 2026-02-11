#include <stdio.h>
#include <esp_random.h>
#include <esp_rom_sys.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef struct {
    uint32_t workerID;
    uint32_t workTimeMS;      // how much time to load CPU in milli-seconds
    uint32_t restTimeMS;      // how much time not to load CPU in milli-seconds  
} workerInfo_t;

void worker(void* pvParameters) {
    const workerInfo_t* info = pvParameters;

    for ( ;; ) {
        printf("[Worker #%lu] Work started...\n", info->workerID);
        
        esp_rom_delay_us(info->workTimeMS * 1000);      // multiplying by 1000 since we have to convert from ms to us

        printf("[Worker #%lu] Work ended...\n", info->workerID);

        vTaskDelay(pdMS_TO_TICKS(info->restTimeMS));
    }
}

void app_main(void)
{
    TaskHandle_t worker1 = NULL;
    TaskHandle_t worker2 = NULL;

    workerInfo_t workerInfo1 = {
        .restTimeMS = 200,
        .workTimeMS = 100,
        .workerID = 1
    };

    workerInfo_t workerInfo2 = {
        .restTimeMS = 100,
        .workTimeMS = 400,
        .workerID = 2
    };

    xTaskCreate(
        worker,
        "WORKER1",
        2048,
        &workerInfo1,
        1,
        &worker1);
    
    xTaskCreate(
        worker,
        "WORKER2",
        2048,
        &workerInfo2,
        1,
        &worker2);
    
    char stats_buffer[1024];

    for( ;; ) {
        vTaskGetRunTimeStats(stats_buffer);
        printf("\nTask Name\tAbs Time\tTime %%\n");
        printf("------------------------------------------\n");
        printf("%s", stats_buffer);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
