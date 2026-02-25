#ifndef SNAKE_GAME_H
#define SNAKE_GAME_H

#include "led_manager.h"
#include "joystick_reader.h"

#define SNAKE_GO_UP_BIT JOYSTICK_Y0_BIT
#define SNAKE_GO_RIGHT_BIT JOYSTICK_X0_BIT
#define SNAKE_GO_DOWN_BIT JOYSTICK_Y4095_BIT
#define SNAKE_GO_LEFT_BIT JOYSTICK_X4095_BIT
#define SNAKE_PAUSE_BIT JOYSTICK_SW_PRESSED_BIT

#define SNAKE_COLOR_HEAD_IDX 0
#define SNAKE_COLOR_BODY_IDX 1
#define SNAKE_COLOR_TARGET_IDX 2

#define SNAKE_DEFAULT_HEAD_COLOR    \
    {                               \
        .red = 100,                 \
        .green = 0,                 \
        .blue = 0,                  \
    }

#define SNAKE_DEFAULT_BODY_COLOR    \
    {                               \
        .red = 33,                  \
        .green = 33,                \
        .blue = 33,                 \
    }

#define SNAKE_DEFAULT_TARGET_COLOR  \
    {                               \
        .red = 0,                   \
        .green = 100,               \
        .blue = 0,                  \
    }

typedef struct {
    uint8_t x;
    uint8_t y;
} pixel_t;

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} color_t;

typedef struct {
    TaskHandle_t task;
    led_manager_handle_t lm;

    // array that stores form of snake using struct "pixel"
    dynamic_array_t shape;

    // where target is located
    pixel_t target;

    // current snake direction
    uint8_t curr_dir;

    // colors for snake body, snake head and target
    color_t color_scheme[3];

    // to pause a game
    bool pause;
} snake_handle_t;

void init_snake(snake_handle_t* snake, EventGroupHandle_t eg);
void snake_start_game(snake_handle_t* snake);

#endif