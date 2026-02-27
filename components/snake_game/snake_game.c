#include <stdio.h>
#include "snake_game.h"
#include "esp_random.h"
#include "led_manager.h"

#include "wifi_snake_controller.h"

#define MATRIX_X_SIZE 8
#define MATRIX_Y_SIZE 8

#define MATRIX_LED_GPIO 17
#define MATRIX_LED_UPDATE_PERIOD 500
#define MATRIX_LED_COUNT 64

#define SNAKE_HEAD_IDX 0
#define SNAKE_BASIC_LEN 3

#define SNAKE_LENGTH(psnake) ((psnake)->shape.curr_size)

// access to snake head x
#define SNAKE_HEAD_X(psnake) (((pixel_t*)da_get_item(&((psnake)->shape), SNAKE_HEAD_IDX))->x)

// access to snake head y
#define SNAKE_HEAD_Y(psnake) (((pixel_t*)da_get_item(&((psnake)->shape), SNAKE_HEAD_IDX))->y)

// pointer to specific snake pixel
#define SNAKE_PX(psnake, idx) ((pixel_t*)da_get_item(&((snake)->shape), (idx)))

// access to specific snake pixel x
#define SNAKE_PX_X(psnake, idx) (SNAKE_PX(psnake, idx)->x)

// access to specific snake pixel y
#define SNAKE_PX_Y(psnake, idx) (SNAKE_PX(psnake, idx)->y)

// convert pixel from (x, y) to a single number for matrix
#define CONVERT_TO_MATRIX_PX(x, y) (((y) * MATRIX_X_SIZE) + (x))

// access to snake tail x
#define SNAKE_TAIL_X(psnake) (((pixel_t*)da_get_item(&((psnake)->shape), SNAKE_LENGTH(psnake) - 1))->x)

// access to snake tail y
#define SNAKE_TAIL_Y(psnake) (((pixel_t*)da_get_item(&((psnake)->shape), SNAKE_LENGTH(psnake) - 1))->y)

// random x position, not exceeding borders by snake length
#define RANDOM_TAIL_X (SNAKE_BASIC_LEN - 1) + (esp_random() % (MATRIX_X_SIZE - ((SNAKE_BASIC_LEN - 1) * 2)))

// random y position, not exceeding borders by snake length
#define RANDOM_TAIL_Y (SNAKE_BASIC_LEN - 1) + (esp_random() % (MATRIX_Y_SIZE - ((SNAKE_BASIC_LEN - 1) * 2)))

// random x position within matrix
#define RANDOM_PX_X esp_random() % MATRIX_X_SIZE

// random y position within matrix
#define RANDOM_PX_Y esp_random() % MATRIX_Y_SIZE

// access to snake head color
#define SNAKE_HEAD_COLOR(psnake) (psnake->color_scheme[SNAKE_COLOR_HEAD_IDX])

// access to snake body color
#define SNAKE_BODY_COLOR(psnake) (psnake->color_scheme[SNAKE_COLOR_BODY_IDX])

// access to target color
#define SNAKE_TARGET_COLOR(psnake) (psnake->color_scheme[SNAKE_COLOR_TARGET_IDX])

#define ALL_SNAKE_MOVES_BITS \
    SNAKE_GO_UP_BIT | SNAKE_GO_RIGHT_BIT | SNAKE_GO_DOWN_BIT | SNAKE_GO_LEFT_BIT | SNAKE_GO_UP_BIT | SNAKE_PAUSE_BIT

static const char TAG[] = "SNAKE_GAME";

// if head head position exceeds border, it appears on the other side
static void update_head(const snake_handle_t snake) {
    if (snake->curr_dir == SNAKE_GO_UP_BIT) {
        if (SNAKE_HEAD_Y(snake) == 0) {
            SNAKE_HEAD_Y(snake) = MATRIX_Y_SIZE - 1;
        }
        else {
            SNAKE_HEAD_Y(snake) -= 1;
        }
    }
    else if (snake->curr_dir == SNAKE_GO_RIGHT_BIT) {
        if (SNAKE_HEAD_X(snake) == MATRIX_X_SIZE - 1) {
            SNAKE_HEAD_X(snake) = 0;
        }
        else {
            SNAKE_HEAD_X(snake) += 1;
        }
    }
    else if (snake->curr_dir == SNAKE_GO_DOWN_BIT) {
        if (SNAKE_HEAD_Y(snake) == MATRIX_Y_SIZE - 1) {
            SNAKE_HEAD_Y(snake) = 0;
        }
        else {
            SNAKE_HEAD_Y(snake) += 1;
        }
    }
    else if (snake->curr_dir == SNAKE_GO_LEFT_BIT) {
        if (SNAKE_HEAD_X(snake) == 0) {
            SNAKE_HEAD_X(snake) = MATRIX_X_SIZE - 1;
        }
        else {
            SNAKE_HEAD_X(snake) -= 1;
        }
    }
}

static uint8_t random_dir() {
    uint8_t random_number = esp_random() % 4;
    switch (random_number)
    {
    case 0:
        return SNAKE_GO_UP_BIT;
        break;
    case 1:
        return SNAKE_GO_RIGHT_BIT;
        break;
    case 2:
        return SNAKE_GO_DOWN_BIT;
        break;
    case 3:
        return SNAKE_GO_LEFT_BIT;
        break;
    }

    return SNAKE_GO_RIGHT_BIT;
}

static void form_basic_snake(snake_handle_t snake) {
    snake->curr_dir = random_dir();

    // using RANDOM_HEAD_X
    pixel_t tail = {
        .x = RANDOM_TAIL_X,
        .y = RANDOM_TAIL_Y,
    };
    da_push_back(&(snake->shape), &tail);

    // creating snake from tail to head
    for (size_t i = 0; i < SNAKE_BASIC_LEN - 1; ++i) {
        da_insert_item(&(snake->shape), SNAKE_PX(snake, SNAKE_HEAD_IDX),  SNAKE_HEAD_IDX);
        update_head(snake);
    }
}

static void update_target(snake_handle_t snake) {
    bool collision;

    // finding position that is not in snake shape
    do {
        collision = false;
        snake->target.y = RANDOM_PX_Y;
        snake->target.x = RANDOM_PX_X;
        for (size_t i = 0; i < snake->shape.curr_size; ++i) {
            if (snake->target.x == SNAKE_PX_X(snake, i)
                && snake->target.y == SNAKE_PX_Y(snake, i)) {
                collision = true;
                break;
            }
        }
    } while (collision);
}

void init_snake(snake_handle_t snake, EventGroupHandle_t eg) {
    assert(eg != NULL);
    
    snake->lm.event_group = eg;
    da_create_array(&(snake->shape), sizeof(pixel_t));
    form_basic_snake(snake);
    update_target(snake);
    snake->color_scheme[SNAKE_COLOR_HEAD_IDX] = (color_t)SNAKE_DEFAULT_HEAD_COLOR;
    snake->color_scheme[SNAKE_COLOR_BODY_IDX] = (color_t)SNAKE_DEFAULT_BODY_COLOR;
    snake->color_scheme[SNAKE_COLOR_TARGET_IDX] = (color_t)SNAKE_DEFAULT_TARGET_COLOR;

    led_strip_config_t strip_config = {
        .strip_gpio_num = MATRIX_LED_GPIO,
        .max_leds = MATRIX_LED_COUNT,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags = {
            .invert_out = false,
        }
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .mem_block_symbols = 64,
        .flags = {
            .with_dma = false,
        }
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &(snake->lm.strip)));
    ESP_ERROR_CHECK(led_strip_clear(snake->lm.strip));
    ESP_LOGI(TAG, "Matrix initialized on GPIO %d", MATRIX_LED_GPIO);
}

// if new direction is the same or opposite, do nothing, otherwise, update current direction
static void update_direction(snake_handle_t snake, uint8_t new_dir) {
    if (new_dir == 0) {
        return;
    }
    else if (snake->curr_dir == SNAKE_GO_UP_BIT) {
        if (new_dir == SNAKE_GO_UP_BIT || new_dir == SNAKE_GO_DOWN_BIT) {
            return;
        }
    }
    else if (snake->curr_dir == SNAKE_GO_RIGHT_BIT) {
        if (new_dir == SNAKE_GO_RIGHT_BIT || new_dir == SNAKE_GO_LEFT_BIT) {
            return;
        }
    }
    else if (snake->curr_dir == SNAKE_GO_DOWN_BIT) {
        if (new_dir == SNAKE_GO_DOWN_BIT || new_dir == SNAKE_GO_UP_BIT) {
            return;
        }
    }
    else if (snake->curr_dir == SNAKE_GO_LEFT_BIT) {
        if (new_dir == SNAKE_GO_LEFT_BIT || new_dir == SNAKE_GO_RIGHT_BIT) {
            return;
        }
    }
    snake->curr_dir = new_dir;
}

static void update_snake(snake_handle_t snake) {
    // copying last pixel from snake shape in case snake eats target
    pixel_t tail_copy = {
        .x = SNAKE_TAIL_X(snake),
        .y = SNAKE_TAIL_Y(snake),
    };

    // updating snake form begins from tail to the second pixel(if count from head)
    for (size_t i = SNAKE_LENGTH(snake) - 1; i > 0; --i) {
        // just copying position from previous pixel
        *SNAKE_PX(snake, i) = *SNAKE_PX(snake, i - 1);
    }
    update_head(snake);

    for (size_t i = 1; i < SNAKE_LENGTH(snake); ++i) {
        if (SNAKE_HEAD_X(snake) == SNAKE_PX_X(snake, i)
            && SNAKE_HEAD_Y(snake) == SNAKE_PX_Y(snake, i))
        {
            da_erase_all(&(snake->shape));
            form_basic_snake(snake);
        }
    }

    // if snake eats target
    if (SNAKE_HEAD_X(snake) == snake->target.x
        && SNAKE_HEAD_Y(snake) == snake->target.y) {
        da_push_back(&(snake->shape), &tail_copy);
        update_target(snake);
    }
}

static void update_picture(const snake_handle_t snake) {
    led_strip_clear(snake->lm.strip);

    // draw snake
    for(size_t i = 0; i < SNAKE_LENGTH(snake); ++i) {
        if (i == 0) {
            led_strip_set_pixel(snake->lm.strip, CONVERT_TO_MATRIX_PX(SNAKE_PX_X(snake, i), SNAKE_PX_Y(snake, i)),
                SNAKE_HEAD_COLOR(snake).red, SNAKE_HEAD_COLOR(snake).green, SNAKE_HEAD_COLOR(snake).blue);
        }
        else {
            led_strip_set_pixel(snake->lm.strip, CONVERT_TO_MATRIX_PX(SNAKE_PX_X(snake, i), SNAKE_PX_Y(snake, i)),
                SNAKE_BODY_COLOR(snake).red, SNAKE_BODY_COLOR(snake).green, SNAKE_BODY_COLOR(snake).blue);
        }
    }

    // draw target
    led_strip_set_pixel(snake->lm.strip, CONVERT_TO_MATRIX_PX(snake->target.x, snake->target.y),
        SNAKE_TARGET_COLOR(snake).red, SNAKE_TARGET_COLOR(snake).green, SNAKE_TARGET_COLOR(snake).blue);

    led_strip_refresh(snake->lm.strip);
}

static void handle_pause(snake_handle_t snake) {
    xEventGroupSetBits(snake->lm.event_group, SNAKE_RECEIVED_PAUSE_BIT);
    uint32_t bits = 0;
    ESP_LOGI(TAG, "Waiting for resume...");
    while (!(bits & SNAKE_RESUME_BIT)) {
        bits = xEventGroupWaitBits(
            snake->lm.event_group,
            SNAKE_RESUME_BIT,
            pdTRUE,
            pdTRUE,
            portMAX_DELAY
        );
    }
    ESP_LOGI(TAG, "Resumed!");
    xEventGroupSetBits(snake->lm.event_group, SNAKE_RECEIVED_RESUME_BIT);
}

static void event_loop(void* pvParameters) {
    snake_handle_t snake = pvParameters;
    uint32_t bits;

    for ( ;; ) {
        bits = xEventGroupWaitBits(
            snake->lm.event_group,
            SNAKE_PAUSE_BIT,
            pdTRUE,
            pdTRUE,
            pdMS_TO_TICKS(MATRIX_UPDATE_PERIOD)
        );

        if (bits & SNAKE_PAUSE_BIT) {
            handle_pause(snake);
            vTaskDelay(pdMS_TO_TICKS(MATRIX_LED_UPDATE_PERIOD));
            continue;
        }
        
        bits = xEventGroupGetBits(snake->lm.event_group);
        xEventGroupClearBits(snake->lm.event_group, 0xffff);

        if (bits & SNAKE_PAUSE_BIT) {            
            handle_pause(snake);
            continue;
        }

        update_direction(snake, bits);
        update_snake(snake);
        update_picture(snake);
    }
}

void snake_start_game(snake_handle_t snake) {
    xTaskCreate(
        event_loop,
        "SNAKE_TASK",
        8192,
        snake,
        3,
        &(snake->task)
    );
}