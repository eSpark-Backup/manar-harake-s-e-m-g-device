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
#include "driver/timer.h"
#include "esp_log.h"
#include "adc_controller.h"

static const char *TAG = "ADC_READER";

#define TIMER_GROUP    TIMER_GROUP_0
#define TIMER_INDEX    TIMER_0
#define TIMER_DIVIDER  16  // Timer clock divider (80 MHz / 16 = 5 MHz)
#define TIMER_INTERVAL_SEC  0.05  // Sampling interval in seconds


// Interrupt handler for the timer
void IRAM_ATTR timer_isr_handler(void *param) {
    // Clear the interrupt flag
    timer_group_clr_intr_status_in_isr(TIMER_GROUP, TIMER_INDEX);

    // Re-enable the alarm
    timer_group_enable_alarm_in_isr(TIMER_GROUP, TIMER_INDEX);

    read_adc();
}

void adc_timer_init() {
    timer_config_t config = {
        .divider = TIMER_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = true
    };

    timer_init(TIMER_GROUP, TIMER_INDEX, &config);
    timer_set_counter_value(TIMER_GROUP, TIMER_INDEX, 0x00000000);
    timer_set_alarm_value(TIMER_GROUP, TIMER_INDEX, TIMER_INTERVAL_SEC * (TIMER_BASE_CLK / TIMER_DIVIDER));
    timer_enable_intr(TIMER_GROUP, TIMER_INDEX);

    timer_isr_register(TIMER_GROUP, TIMER_INDEX, timer_isr_handler, NULL, ESP_INTR_FLAG_IRAM, NULL);
    timer_start(TIMER_GROUP, TIMER_INDEX);
}

void app_main(void)
{

    esp_err_t r;
    r = initializing_adc();
    assert( r == ESP_OK );

    adc_timer_init();
    while(1)
    {
        printf("\nPrinting ADC Values\n");
        print_adc_values();
        vTaskDelay(50/portTICK_PERIOD_MS);
    }
}
