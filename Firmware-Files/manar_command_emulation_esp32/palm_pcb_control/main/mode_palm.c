#include "mode_palm.h"
#include "manar_uart_hal.h"
#include "esp_log.h"
#include <string.h>

void handle_palm_command(const char *cmd) {
    ESP_LOGW("PALM", "HANDLING PALM COMMAND");
    manar_uart_write(MANAR_UART_TX, cmd, strlen(cmd));
    manar_uart_write(MANAR_UART_TX, "\n", 1);
}
