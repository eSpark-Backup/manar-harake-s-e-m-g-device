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

#define NUMBER_OF_GESTURES (GESTURES_MAX - 1)

static ControlMode current_mode = MODE_UNKNOWN;

int parse_gesture(uint8_t *data, size_t len) {
    // Assuming data is a string representing a number from 1 to 6
    if (len == 0) {
        return -1; // Invalid data length
    }

    // Convert the data to an integer
    int gesture = atoi((char *)data);

    // Check if the gesture is within the valid range (1-6)
    if (gesture >= 1 && gesture <= NUMBER_OF_GESTURES) {
        return gesture;  // Return 0-based index for the gesture
    } else {
        return -1;  // Invalid gesture
    }
}

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
            ESP_LOGE("MODE NOT HANDLED");
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
