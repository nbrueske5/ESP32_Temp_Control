/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "esp_lcd_io_spi.h"
#include "esp_lcd_panel_st7789.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"

/**
 * SPI LCD to ESP32 Inputs
 */
#define LCD_HOST    SPI2_HOST

#define PIN_NUM_MISO -1
#define PIN_NUM_MOSI 5
#define PIN_NUM_CLK  6
#define PIN_NUM_CS   7

#define PIN_NUM_DC   15
#define PIN_NUM_RST  16
#define PIN_NUM_BCKL 17

#define LCD_BK_LIGHT_ON_LEVEL   1
#define PARALLEL_LINES 16
// LCD size
#define LCD_WIDTH  320
#define LCD_HEIGHT 240
// Seven Seg config
#define SEVEN_SEG_THICK 1
#define SEVEN_SEG_LENGTH 5

//TASKS
TaskHandle_t fill_screen_hdl = NULL;

esp_lcd_panel_handle_t panel_handle = NULL;

void init_lcd(void) {

    // 1. Initialize the SPI bus
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = PARALLEL_LINES * 320 * 2 + 8
    };
    spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);

    // 2. Attach the LCD to the SPI bus
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
    .dc_gpio_num = PIN_NUM_DC,
    .cs_gpio_num = PIN_NUM_CS,
    .pclk_hz = 20*1000*1000, //Clock out at 20 MHz
    .lcd_cmd_bits = 8,
    .lcd_param_bits = 8,
    .spi_mode = 0,
    .trans_queue_depth = 10
    };
    esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle);

    // 3. Install the LCD controller driver
    esp_lcd_panel_dev_config_t panel_config = {
    .reset_gpio_num = PIN_NUM_RST,
    .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
    .bits_per_pixel = 16,
    };
    // Create LCD panel handle for ST7789, with the SPI IO device handle
    esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle);

    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);

    esp_lcd_panel_swap_xy(panel_handle, true); // fix orientation

    esp_lcd_panel_disp_on_off(panel_handle, true);

     // Configure backlight
    gpio_config_t bk_gpio_config = {
        .pin_bit_mask = 1ULL << PIN_NUM_BCKL,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    gpio_config(&bk_gpio_config);
    gpio_set_level(PIN_NUM_BCKL, LCD_BK_LIGHT_ON_LEVEL);
}

void fill_screen(void *color)
{
    uint32_t line_buffer[LCD_HEIGHT];
    for (int i = 0; i < LCD_HEIGHT; i++) {
        line_buffer[i] = (uint32_t)color;

    }

    for (int i = 0; i < LCD_WIDTH; i++) {
        esp_lcd_panel_draw_bitmap(panel_handle, i, 0, i+1, LCD_HEIGHT, line_buffer);
    }
    vTaskDelete(NULL);
}

void draw_seven_seg(int size, int x_pos, int y_pos, uint16_t color, bool *data) {
    int segm_length = SEVEN_SEG_LENGTH*size;
    int segm_thickness = SEVEN_SEG_THICK*size;
    // need real size
    if (size <= 0) {
        return;
    } 

    uint16_t line_buffer[SEVEN_SEG_LENGTH * size];
    for (int i = 0; i < SEVEN_SEG_LENGTH * size; i++) {
        line_buffer[i] = color;
    }
    // in a loop to reduce size of line_buffer
    for (int i = 0; i < SEVEN_SEG_THICK*size; i++) {
        if (data[0]) {
            esp_lcd_panel_draw_bitmap(panel_handle,
                x_pos + segm_thickness,
                y_pos + i,
                x_pos + segm_thickness + segm_length,
                y_pos+i+1,
                line_buffer);
        }
        if (data[1]) {
            esp_lcd_panel_draw_bitmap(panel_handle,
                x_pos + segm_thickness + segm_length + i, // x init
                y_pos + segm_thickness, // y init
                x_pos + segm_thickness + segm_length + i + 1, // x final
                y_pos + segm_thickness + segm_length, // y final
                line_buffer);
        }
        if (data[2]) {
            esp_lcd_panel_draw_bitmap(panel_handle, 
                x_pos + segm_thickness + segm_length + i, // x init
                y_pos + 2*segm_thickness + segm_length, // y init
                x_pos + segm_thickness + segm_length + i + 1, // x final
                y_pos + 2*segm_thickness + 2*segm_length, // y final
                line_buffer);
        }
        if (data[3]) {
            esp_lcd_panel_draw_bitmap(panel_handle,
                x_pos + segm_thickness,
                y_pos + 2*segm_thickness + 2*segm_length + i,
                x_pos + segm_thickness + segm_length,
                y_pos + 2*segm_thickness + 2*segm_length + i + 1,
                line_buffer);
        }
        if (data[4]) {
            esp_lcd_panel_draw_bitmap(panel_handle, 
                x_pos + i, // x init
                y_pos + 2*segm_thickness + segm_length, // y init
                x_pos + i + 1, // x final
                y_pos + 2*segm_thickness + 2*segm_length, // y final
                line_buffer);
        }
        if (data[5]) {
            esp_lcd_panel_draw_bitmap(panel_handle,
                x_pos + i, // x init
                y_pos + segm_thickness, // y init
                x_pos + i + 1, // x final
                y_pos + segm_thickness + segm_length, // y final
                line_buffer);
        }
        if (data[6]) {
            esp_lcd_panel_draw_bitmap(panel_handle,
                x_pos + segm_thickness,
                y_pos + segm_length + segm_thickness + i,
                x_pos + segm_thickness + segm_length,
                y_pos + segm_length +segm_thickness + i + 1,
                line_buffer);
        }
    }
    
    
}

void app_main(void)
{
    init_lcd();
    draw_seven_seg(5, 10, 10, 0x0, (bool[]){1,1,1,1,1,1,1}); //Display 0
}
