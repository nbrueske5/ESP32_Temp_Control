#include "LCD_Display.h"
#include "temp_sensor.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include <stdint.h>

const static char *TAG = "main";
#define SCALE 10
#define X_POS 40
#define Y_POS 30

void app_main(void)
{
    init_temp_sensor();
    init_lcd();
    fill_screen(0x0000);
    vTaskDelay(100);

    float avg_voltage = 0;
    float curr_temp = 0;
    float target_temp = 46.00f;
    float temp_offset = 0.20f;

    
    while (1) {
        // Average out the input
        avg_voltage = 0;
        for (int i = 0; i < AVG_SAMPLES; i++) {
            ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC1_CHAN, &adc_raw));
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan_handle, adc_raw, &voltage));
            avg_voltage += voltage;
            vTaskDelay(pdMS_TO_TICKS(AVG_SAMPLES_SPEED_MS / AVG_SAMPLES));
        }
        avg_voltage = avg_voltage / AVG_SAMPLES;
        ESP_LOGI(TAG, "Average Cali voltage: %.2f mV", avg_voltage);
        curr_temp = (avg_voltage - 543) / 10.0; // For TMP36 sensor
        ESP_LOGI(TAG, "Temp: %.2f C", curr_temp); // For TMP36 sensor
        curr_temp = -1*curr_temp; 
        // format voltage
        char buffer[10];
        sprintf(buffer, "%.2f", curr_temp);
        // too hot
        if (curr_temp > target_temp + temp_offset) {
            draw_string_3x5(SCALE, X_POS, Y_POS, 5, RED, buffer);
        // too cold
        } else if (curr_temp < target_temp - temp_offset) {
            draw_string_3x5(SCALE, X_POS, Y_POS, 5, BLUE, buffer);
        // perfect
        } else {
            draw_string_3x5(SCALE, X_POS, Y_POS, 5, GREEN, buffer);
        }
        // give display time to draw
        vTaskDelay(pdMS_TO_TICKS(10));
            
    }

}