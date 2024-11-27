#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "driver/dac.h"
#include "adc_controller.h"

#define ADC2_EXAMPLE_CHANNEL_0          ADC2_CHANNEL_9
#define ADC2_EXAMPLE_CHANNEL_1          ADC2_CHANNEL_8

#define ADC1_EXAMPLE_CHANNEL_0          ADC1_CHANNEL_5 
#define ADC1_EXAMPLE_CHANNEL_1          ADC1_CHANNEL_4
#define ADC1_EXAMPLE_CHANNEL_2          ADC1_CHANNEL_7
#define ADC1_EXAMPLE_CHANNEL_3          ADC1_CHANNEL_6
#define ADC1_EXAMPLE_CHANNEL_4          ADC1_CHANNEL_3
#define ADC1_EXAMPLE_CHANNEL_5          ADC1_CHANNEL_0

#if CONFIG_IDF_TARGET_ESP32
static const adc_bits_width_t width = ADC_WIDTH_BIT_12;
#elif CONFIG_IDF_TARGET_ESP32S2
static const adc_bits_width_t width = ADC_WIDTH_BIT_13;
#endif

#define NUM_ADC 8

uint8_t ADC1_CHANNELS[6] = {ADC1_EXAMPLE_CHANNEL_0, ADC1_EXAMPLE_CHANNEL_1, ADC1_EXAMPLE_CHANNEL_2, ADC1_EXAMPLE_CHANNEL_3, ADC1_EXAMPLE_CHANNEL_4, ADC1_EXAMPLE_CHANNEL_5};
uint8_t ADC2_CHANNELS[2] = {ADC2_EXAMPLE_CHANNEL_0, ADC2_EXAMPLE_CHANNEL_1};

esp_err_t initialize_adc1()
{
    gpio_num_t adc_gpio_num;
    esp_err_t r = ESP_FAIL;
    printf("adc1_init...\n");
    for(int i = 0; i < sizeof(ADC1_CHANNELS); i++)
    {
        r = adc1_pad_get_io_num( ADC1_CHANNELS[i], &adc_gpio_num );
        assert( r == ESP_OK );

        printf("ADC1 channel %d @ GPIO %d\n", ADC1_CHANNELS[i], adc_gpio_num);


        //be sure to do the init before using adc2.
        r = adc1_config_channel_atten( ADC1_CHANNELS[i], ADC_ATTEN_11db );

        vTaskDelay(2 * portTICK_PERIOD_MS);
    }
    return r;
}

esp_err_t initialize_adc2()
{
    gpio_num_t adc_gpio_num;
    esp_err_t r = ESP_FAIL;
    printf("adc2_init...\n");
    for(int i = 0; i < sizeof(ADC2_CHANNELS); i++)
    {
        r = adc2_pad_get_io_num( ADC2_CHANNELS[i], &adc_gpio_num );
        assert( r == ESP_OK );

        printf("ADC2 channel %d @ GPIO %d\n", ADC2_CHANNELS[i], adc_gpio_num);


        //be sure to do the init before using adc2.
        r = adc2_config_channel_atten( ADC2_CHANNELS[i], ADC_ATTEN_11db );

        vTaskDelay(2 * portTICK_PERIOD_MS);
    }
    return r;
}

esp_err_t initializing_adc(void)
{
    esp_err_t r = ESP_FAIL;

    r = initialize_adc2();
    assert( r == ESP_OK );
    
    r = initialize_adc1();
    assert( r == ESP_OK );

    return r;
}

void read_adc2(void)
{
    int     read_raw;
    esp_err_t r;

    for(int i = 0; i < sizeof(ADC2_CHANNELS); i++)
    {
        r = adc2_get_raw( ADC2_CHANNELS[i], width, &read_raw);
        if ( r == ESP_OK ) {
            // printf("ADC2 CHANNEL %d: %d\n", ADC2_CHANNELS[i], read_raw );
            printf("%d,", read_raw );
        } else if ( r == ESP_ERR_INVALID_STATE ) {
            printf("%s: ADC2 not initialized yet.\n", esp_err_to_name(r));
        } else if ( r == ESP_ERR_TIMEOUT ) {
            //This can not happen in this example. But if WiFi is in use, such error code could be returned.
            printf("%s: ADC2 is in use by Wi-Fi.\n", esp_err_to_name(r));
        } else {
            printf("%s\n", esp_err_to_name(r));
        }

        vTaskDelay( 10/portTICK_PERIOD_MS );
    }
}

void read_adc1(void)
{
    int     read_raw;

    printf("start conversion.\n");
    for(int i = 0; i < sizeof(ADC1_CHANNELS); i++)
    {
        read_raw = adc1_get_raw( ADC1_CHANNELS[i]);
            // printf("ADC1 CHANNEL %d: %d\n", ADC1_CHANNELS[i], read_raw );
            printf("%d,", read_raw );
        vTaskDelay( 10/portTICK_PERIOD_MS );
    }

}

void read_adc(void)
{
    read_adc1();
    read_adc2();
    printf("%d,%d,\n", 0, 4096);
}