#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "manar_uart_hal.h"
#include "mode_palm.h"

static const char *TAG = "uart_events";

typedef enum {
    MODE_PALM,
    MODE_MOUSE,
    MODE_GAMEPAD,
    MODE_VR,
    MODE_UNKNOWN
} ControlMode;

static ControlMode current_mode = MODE_UNKNOWN;

void my_uart_callback(uint8_t *data, size_t len) {
    int gesture = parse_gesture(data, len);
    if (gesture == -1) {

        ESP_LOGE(TAG, "Invalid Gesture");
        return;
    }
    switch (current_mode) {
        case MODE_PALM:
            handle_palm_command(gesture);
            break;
        case MODE_MOUSE:
            // handle_mouse_command(gesture);
            break;
        case MODE_GAMEPAD:
            // handle_gamepad_command(gesture);
            break;
        case MODE_VR:
            // handle_vr_command(gesture);
            break;
        default:
            ESP_LOGE(TAG, "MODE NOT HANDLED");
            break;
    }
}

void app_main(void)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);

    current_mode = MODE_PALM;

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
