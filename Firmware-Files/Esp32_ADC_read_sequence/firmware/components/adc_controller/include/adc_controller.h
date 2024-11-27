#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "driver/dac.h"

#define ADC2_EXAMPLE_CHANNEL_0          ADC2_CHANNEL_9
#define ADC2_EXAMPLE_CHANNEL_1          ADC2_CHANNEL_8

#define ADC1_EXAMPLE_CHANNEL_0          ADC1_CHANNEL_5 
#define ADC1_EXAMPLE_CHANNEL_1          ADC1_CHANNEL_4
#define ADC1_EXAMPLE_CHANNEL_2          ADC1_CHANNEL_7
#define ADC1_EXAMPLE_CHANNEL_3          ADC1_CHANNEL_6
#define ADC1_EXAMPLE_CHANNEL_4          ADC1_CHANNEL_3
#define ADC1_EXAMPLE_CHANNEL_5          ADC1_CHANNEL_0

#define TIMER_GROUP    TIMER_GROUP_0
#define TIMER_INDEX    TIMER_0
#define TIMER_DIVIDER  16  // Timer clock divider (80 MHz / 16 = 5 MHz)
#define TIMER_INTERVAL_SEC  0.05  // Sampling interval in seconds

#define ADC_CON_ATTEN ADC_ATTEN_11db
#define NUM_ADC_CHANNELS 8

#if CONFIG_IDF_TARGET_ESP32
static const adc_bits_width_t width = ADC_WIDTH_BIT_12;
#elif CONFIG_IDF_TARGET_ESP32S2
static const adc_bits_width_t width = ADC_WIDTH_BIT_13;
#endif


esp_err_t initializing_adc(void);
void adc_start();
void read_adc(void);
void print_adc_values();
