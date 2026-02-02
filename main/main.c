#include "LCD_Display.h"
#include "temp_sensor.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include <stdint.h>
#include "esp_now.h"
#include "esp_rtc_time.h"
#include "Pixel_Array_Presets.h"

const static char *TAG = "main";
#define SCALE 10
#define X_POS 40
#define Y_POS 30

void app_main(void)
{
    init_temp_sensor();
    init_lcd();
    fill_screen(0x0000);
    createReadSensorTask();


    float target_temp = 27.00f;
    float temp_offset = 0.20f;

    bool resetTimer = false;
    bool fanA = true;
    int startTime = xTaskGetTickCount() * portTICK_PERIOD_MS * 1000;

    while (1) {
        if (resetTimer) {
            ESP_LOGI(TAG, "reset");
            startTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
            resetTimer = false;
        }
        if (xTaskGetTickCount() * portTICK_PERIOD_MS - startTime > 300) {
            if (fanA) {
                draw_pixel_map(5, 100, 100, 16, 16, WHITE, preset_fanA_16x16);
            } else {
                draw_pixel_map(5, 100, 100, 16, 16, WHITE, preset_fanB_16x16);
            }
            fanA = !fanA;
            resetTimer = true;
        }
       // format voltage
        char buffer[10];
        sprintf(buffer, "%.2fC", curr_temp);
        if (curr_temp > target_temp + temp_offset) {
            draw_string_3x5(SCALE, X_POS, Y_POS, 6, RED, buffer);
        // too cold
        } else if (curr_temp < target_temp - temp_offset) {
            draw_string_3x5(SCALE, X_POS, Y_POS, 6, BLUE, buffer);
        // perfect
        } else {
            draw_string_3x5(SCALE, X_POS, Y_POS, 6, GREEN, buffer);
        }
        // give display time to draw
        //vTaskDelay(pdMS_TO_TICKS(10));
        
    }

}