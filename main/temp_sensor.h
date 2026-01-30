#include "esp_adc/adc_oneshot.h"


//ADC1 Channel 3
#define ADC1_CHAN           ADC_CHANNEL_2
#define ADC_ATTEN           ADC_ATTEN_DB_12

#define AVG_SAMPLES 20
#define AVG_SAMPLES_SPEED_MS 2000 // Total delay time for averaging samples

extern adc_cali_handle_t adc1_cali_chan_handle;
extern adc_oneshot_unit_handle_t adc1_handle;

extern int adc_raw;
extern int voltage;

void init_temp_sensor(void);