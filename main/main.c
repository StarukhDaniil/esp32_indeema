#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_random.h>

typedef struct {
    uint32_t deviceID;
    uint32_t measurementID;
    float temperature;
} DataPackage_t;

QueueHandle_t xSensorQueue;

void task_sender(void* pvParameters) {
    DataPackage_t dataToSend;
    dataToSend.deviceID = 101;
    dataToSend.measurementID = 0;

    for ( ;; ) {
        dataToSend.temperature = 20.0 + (float)(esp_random() % 100) / 10.0;
        ++dataToSend.measurementID;
        printf("[Sender] Sending measurement #%lu (Temp: %.2f)\n",
            dataToSend.measurementID, dataToSend.temperature);
        
        if (xQueueSend(xSensorQueue, &dataToSend, portMAX_DELAY) != pdPASS) {
            printf("[SENDER] Failed to send");
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void task_receiver(void* pvParameters) {
    DataPackage_t receivedData;
    for ( ;; ) {
        if (xQueueReceive(xSensorQueue, &receivedData, portMAX_DELAY) == pdTRUE) {
            printf("[Receiver] Received data! Device: %lu | ID: %lu | Temp: %.2f\n",
                receivedData.deviceID, receivedData.measurementID, receivedData.temperature);
        }
    }
}

void app_main(void)
{
    xSensorQueue = xQueueCreate(5, sizeof(DataPackage_t));
    if (xSensorQueue == NULL) {
        printf("Error while creating a queue!\n");
        return;
    }

    xTaskCreatePinnedToCore(task_sender, "SENDER", 2048, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(task_receiver, "RECEIVER", 2048, NULL, 1, NULL, 1);
}
