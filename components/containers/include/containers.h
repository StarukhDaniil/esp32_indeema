#ifndef CONATINERS_H
#define CONATINERS_H

#include "stdint.h"

#define DA_STD_CONTAINER_SIZE 10

#define GET_BIT_FROM_HANDLE_ARR(handle_ptr, idx) (((bit_cb_pair_t*)(da_get_item(&((handle_ptr)->bit_cbs_arr), (idx))))->bit)
#define CALL_CB_FROM_HANDLE_ARR(handle_ptr, idx) (((bit_cb_pair_t*)(da_get_item(&((handle_ptr)->bit_cbs_arr), (idx))))->cb(handle_ptr))

typedef struct {
    uint8_t* pdata;
    size_t curr_size;
    size_t max_size;
    size_t data_size;
} dynamic_array_t;

typedef struct {
    uint32_t bit;
    void(*cb)(void*);
} bit_cb_pair_t;

// da is Dynamic Array

// returns NULL if error, otherwise, returens ptr to arr
dynamic_array_t* da_create_array(dynamic_array_t* arr, size_t data_size);

// returns NULL if error, otherwise, returns ptr to inserted data
void* da_push_back(dynamic_array_t* arr, void* data);

// returns NULL if error, otherwise, returns ptr to inserted data
void* da_insert_item(dynamic_array_t* arr, void* data, size_t idx);

// returns NULL if error, otherwise, returens ptr to arr
dynamic_array_t* da_erase_item(dynamic_array_t* arr, size_t idx);

// returns NULL if error, otherwise, returens ptr to arr
dynamic_array_t* da_erase_all(dynamic_array_t* arr);

// returns NULL if error, otherwise, returens ptr to arr
dynamic_array_t* da_free_arr(dynamic_array_t* arr);

// returns ptr to item
void* da_get_item(const dynamic_array_t* arr, size_t idx);

#endif // CONTAINERS_H