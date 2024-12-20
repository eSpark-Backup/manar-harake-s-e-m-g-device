/* ADC2 Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "adc_controller.h"
#include "ble_service.h"

static const char *TAG = "ADC_READER_PHASE_2";

adc_log_cb_t printf_logging(uint8_t* data, uint16_t len)
{
    printf("%s", data);
    return ESP_OK;
}

void app_main(void)
{
    esp_err_t r;
    
    ble_service_init();

    r = initialize_adc();
    assert( r == ESP_OK );

    // adc_set_logging_callback((adc_log_cb_t)printf_logging);
    adc_set_logging_callback((adc_log_cb_t)ble_server_send_data);
    adc_sampling_start();


    while(1)
    {
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
}
