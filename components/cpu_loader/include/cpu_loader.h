#ifndef CPU_LOADER_H
#define CPU_LOADER_H

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

void worker(void* pvParameters);

void my_test_load(void); // function that creates 2 task, makes load on cpu and logs cpu load

#endif