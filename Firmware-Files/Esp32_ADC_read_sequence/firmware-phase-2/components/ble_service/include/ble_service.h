#include "esp_err.h"

esp_err_t ble_server_send_data(uint8_t *data_ptr, uint16_t data_len);
esp_err_t ble_service_init(void);