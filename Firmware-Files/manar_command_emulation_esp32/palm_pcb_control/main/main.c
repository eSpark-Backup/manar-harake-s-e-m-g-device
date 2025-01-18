#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "manar_uart_hal.h"
#include "mode_palm.h"

static const char *TAG = "uart_events";

void my_uart_callback(uint8_t *data, size_t len) {
    ESP_LOGW("MAIN", "Received data: %.*s \n\n", len, data);
    handle_palm_command((const char*)data);
}

void app_main(void)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);

    manar_uart_config_t command_uart_config = {.uart = MANAR_UART_TX, .baudrate = 115200};
    manar_uart_init(command_uart_config, NULL);
    vTaskDelay(1000/portTICK_PERIOD_MS);

    command_uart_config.uart = MANAR_UART_RX;
    manar_uart_init(command_uart_config, my_uart_callback);
    while(1)
    {
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}
