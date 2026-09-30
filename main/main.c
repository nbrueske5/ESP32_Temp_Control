#include "LCD_Display.h"
#include "esp_attr.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "hal/gpio_types.h"
#include "portmacro.h"
#include "temp_sensor.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include <stdint.h>
#include "esp_now.h"
#include "esp_rtc_time.h"
#include "Pixel_Array_Presets.h"
#include "Communication.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

/* MACROS */
#define SCALE 10
#define X_POS 40
#define Y_POS 30
#define TASK_STACK_SIZE (configMINIMAL_STACK_SIZE *5 )

#define NUM_BUTTONS 4
#define BUTTON_DEBOUNCE_NUM 5

#define BTN_B
#define BTN_C
#define BTN_D

#define BTN_U
#define BTN_R
#define BTN_D
#define BTN_L

/* Global Variables */
TaskHandle_t TaskHandle_Input_Monitor = NULL;
TaskHandle_t TaskHandle_Buttons = NULL;
QueueHandle_t QueueHandle_Input_state = NULL;

static gpio_num_t button_num[] = {GPIO_NUM_1, GPIO_NUM_39, GPIO_NUM_42, GPIO_NUM_41};
static uint64_t button_mask[] = {(1ULL << 1), (1ULL << 39), (1ULL << 42), (1ULL << 41)};
static uint8_t button_values = 0;
typedef struct {
    uint16_t buttons
} input_state_t;

/* Function Delcarations */
static void createTasks();
static void rtos_inputMonitor(void *arg);
static void rtos_buttons(void *arg);

void app_main(void)
{
    
    nvs_flash_init();
    espnow_init();
    init_lcd();
    fill_screen(0x0000);
    /* Buttons */
    for (int i = 0; i < NUM_BUTTONS; i++) {
        const gpio_config_t btn = {
            .pin_bit_mask = button_mask[i],
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = true,
            .intr_type = GPIO_INTR_NEGEDGE
        };
        gpio_config(&btn);
    };
    createTasks();
}

static void createTasks() {

    QueueHandle_Input_state = xQueueCreate(1, sizeof(input_state_t));

    xTaskCreate(
        rtos_inputMonitor,
        "Input Monitor",
        TASK_STACK_SIZE,
        NULL,
        1,
        &TaskHandle_Input_Monitor
    );

    xTaskCreate(
        rtos_buttons,
        "buttons",
        TASK_STACK_SIZE,
        NULL,
        1,
        &TaskHandle_Buttons
    );


}

static void rtos_inputMonitor(void *arg) {
    static uint16_t btn_count_pressed[NUM_BUTTONS] = {0};
    static uint16_t btn_count_not_pressed[NUM_BUTTONS] = {0};
    static uint16_t task_count;
    bool is_pressed[NUM_BUTTONS] = {0};
    input_state_t input_state = {0};

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1));
        task_count++;
        if (task_count >= 1000) {
            printf("TASK\n");
            task_count = 0;
        }
        // for each button
        for (int i = 0; i < NUM_BUTTONS; i++) {
            // if pressed for longer than debounce time and not already counted as pressed -> count as pressed
            if (gpio_get_level(button_num[i]) == 0) {
                btn_count_pressed[i]++;
                btn_count_not_pressed[i] = 0;

                if (btn_count_pressed[i] >= BUTTON_DEBOUNCE_NUM && !is_pressed[i]) {
                    // set pressed
                    is_pressed[i] = true;
                    input_state.buttons |= (1 << i);
                    xQueueOverwrite(QueueHandle_Input_state, &input_state); // send update values
                    //printf("Pressed %d\n", i);
                }
            } else if (gpio_get_level(button_num[i]) == 1){
                btn_count_not_pressed[i]++;
                btn_count_pressed[i] = 0;
                if (btn_count_not_pressed[i] >= BUTTON_DEBOUNCE_NUM && is_pressed[i]) {
                    // set not pressed
                    is_pressed[i] = false;
                    input_state.buttons &= ~(1 << i);
                    xQueueOverwrite(QueueHandle_Input_state, &input_state); // send update values
                    //printf("Let Go %d\n", i);

                }
            } else {
                btn_count_not_pressed[i] = 0;
                btn_count_pressed[i] = 0;
            }
        }
    }
}

static void rtos_buttons(void *arg) {
    input_state_t input_state_old = {0};
    input_state_t input_state_new = {0};

    while (1) {
        xQueuePeek(QueueHandle_Input_state, &input_state_new, portMAX_DELAY);
        for (int i = 0; i < NUM_BUTTONS; i++) {
            // if a button state changed -> do something
            if ((input_state_old.buttons & (1 << i)) != (input_state_new.buttons & (1 << i))) {
                int newState = (input_state_new.buttons & (1 << i)) >> i;
                printf("button %d: %d\n", i, newState);
            }
        }
        input_state_old = input_state_new;
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}