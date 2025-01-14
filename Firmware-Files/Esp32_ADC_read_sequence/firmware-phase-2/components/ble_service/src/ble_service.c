#include <stdint.h>
#include <stdio.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "freertos/task.h"
#include "nimble-nordic-uart.h"

bool connected = false;
bool paired = true;

void echoTask(void *parameter) {
  static char mbuf[CONFIG_NORDIC_UART_MAX_LINE_LENGTH + 1];

  for (;;) {
    size_t item_size;
    if (nordic_uart_rx_buf_handle) {
      const char *item = (char *)xRingbufferReceive(nordic_uart_rx_buf_handle, &item_size, portMAX_DELAY);

      if (item) {
        int i;
        for (i = 0; i < item_size; ++i) {
          if (item[i] >= 'a' && item[i] <= 'z')
            mbuf[i] = item[i] - 0x20;
          else
            mbuf[i] = item[i];
        }
        mbuf[item_size] = '\0';

        nordic_uart_sendln(mbuf);
        puts(mbuf);
        vRingbufferReturnItem(nordic_uart_rx_buf_handle, (void *)item);
      }
    } else {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
  }

  vTaskDelete(NULL);
}

static void __nordic_uart_cb(enum nordic_uart_callback_type callback_type)
{
    printf("NORDIC CALLBACK CALLED");
}

esp_err_t ble_service_init(void) {
  nordic_uart_start("Nordic UART", __nordic_uart_cb);
  xTaskCreate(echoTask, "echoTask", 5000, NULL, 1, NULL);
  return ESP_OK;
}

bool ble_service_is_connected(void) 
{
    // Replace with actual implementation to check BLE connection status
    return connected;
}

bool ble_service_is_paired(void) 
{
    // Replace with actual implementation to check BLE pairing status
    return paired;
}

esp_err_t ble_service_send_data(uint8_t *data_ptr, uint16_t data_len)
{
    if (data_ptr == NULL || data_len == 0) {
        return ESP_ERR_INVALID_ARG; 
    }

    uint8_t temp_buffer[data_len + 1];
    if (temp_buffer == NULL) {
        return ESP_ERR_NO_MEM; 
    }

    memcpy(temp_buffer, data_ptr, data_len);
    temp_buffer[data_len] = '\0';

    nordic_uart_send(temp_buffer);

    return ESP_OK;
}
