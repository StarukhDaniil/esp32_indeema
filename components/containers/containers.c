#include <stdio.h>
#include "containers.h"
#include "malloc.h"
#include "string.h"
#include "esp_log.h"

// calculate how many bytes to move when erasing item
#define CALC_ERASE_BYTES_TO_MOVE(parr, idx) (arr)->data_size * ((arr)->curr_size - ((idx) + 1))

// calculate how many bytes to move when inserting item
#define CALC_INSERT_BYTES_TO_MOVE(parr, idx) (parr)->data_size * ((parr)->curr_size - (idx))

static const char* TAG = "CONTAINERS";

dynamic_array_t* da_create_array(dynamic_array_t* arr, size_t data_size) {
    if (!arr) return NULL;

    arr->pdata = malloc(data_size * DA_STD_CONTAINER_SIZE);
    if (arr->pdata == NULL) {
        return NULL;
    }

    arr->data_size = data_size;
    arr->curr_size = 0;
    arr->max_size = DA_STD_CONTAINER_SIZE;

    return arr;
}

void* da_push_back(dynamic_array_t* arr, void* data) {
    if (!arr || !data) return NULL;

    // if da_free_arr was called
    if (arr->pdata == NULL) {
        arr->pdata = malloc(arr->data_size * arr->max_size);
        if (arr->pdata == NULL) {
            return NULL;
        }
    }

    // if there is a need to expand allocated memory
    if (!(arr->curr_size < arr->max_size)) {
        uint8_t* new_pdata = realloc(arr->pdata, (arr->max_size + 10) * arr->data_size);
        if (new_pdata == NULL) {
            return NULL;
        }
        arr->pdata = new_pdata;
        arr->max_size += 10;
    }

    memcpy(arr->pdata + (arr->curr_size * arr->data_size), data, arr->data_size);
    ++(arr->curr_size);

    return arr->pdata + ((arr->curr_size - 1) * arr->data_size);
}

void* da_insert_item(dynamic_array_t* arr, void* data, size_t idx) {
    if (!arr || !data) return NULL;

    if (idx > arr->curr_size) {
        return NULL;
    }

    // if there is a need to expand allocated memory
    if (!(arr->curr_size < arr->max_size)) {
        uint8_t* new_pdata = realloc(arr->pdata, (arr->max_size + 10) * arr->data_size);
        if (new_pdata == NULL) {
            return NULL;
        }
        arr->pdata = new_pdata;
        arr->max_size += 10;
    }

    memmove(arr->pdata + (idx + 1) * arr->data_size, arr->pdata + idx * arr->data_size, CALC_INSERT_BYTES_TO_MOVE(arr, idx));

    if (((uint8_t*)data < arr->pdata + (idx * arr->data_size)) || ((uint8_t*)data >= arr->pdata + arr->curr_size * arr->data_size)) {
        memcpy(arr->pdata + (idx * arr->data_size), data, arr->data_size);
    }
    else {
        memcpy(arr->pdata + (idx * arr->data_size), (uint8_t*)data + arr->data_size, arr->data_size);
    }

    ++(arr->curr_size);

    return arr->pdata + idx * arr->data_size;
}

dynamic_array_t* da_erase_item(dynamic_array_t* arr, size_t idx) {
    if (!arr) return NULL;
    if (idx > arr->curr_size) return NULL;

    memmove(arr->pdata + idx * arr->data_size, arr->pdata + (idx + 1) * arr->data_size, CALC_ERASE_BYTES_TO_MOVE(arr, idx));

    --(arr->curr_size);
    if (arr->curr_size < (arr->max_size - (DA_STD_CONTAINER_SIZE + (DA_STD_CONTAINER_SIZE / 2)))) {
        uint8_t* new_pdata = realloc(arr->pdata, (arr->max_size - DA_STD_CONTAINER_SIZE));
        if (new_pdata == NULL) {
            return NULL;
        }
        arr->pdata = new_pdata;
        arr->max_size -= DA_STD_CONTAINER_SIZE;
    }

    return arr;
}

dynamic_array_t* da_erase_all(dynamic_array_t* arr) {
    if (arr == NULL || arr->pdata == NULL) return NULL;

    void* new_pdata = realloc(arr->pdata, DA_STD_CONTAINER_SIZE * arr->data_size);
    if (new_pdata == NULL) {
        return NULL;
    }
    arr->pdata = new_pdata;
    arr->curr_size = 0;
    arr->max_size = DA_STD_CONTAINER_SIZE;
    return arr;
}

dynamic_array_t* da_free_arr(dynamic_array_t* arr) {
    if (arr == NULL || arr->pdata == NULL) return NULL;

    free(arr->pdata);
    arr->pdata = NULL;
    arr->curr_size = 0;
    arr->max_size = DA_STD_CONTAINER_SIZE;

    return arr;
}

void* da_get_item(const dynamic_array_t* arr, size_t idx) {
    return arr->pdata + (idx * arr->data_size);
}