#ifndef __WIFI_H
#define __WIFI_H
#include <esp_err.h>
#include <stdbool.h>

// Define WiFi operation modes
typedef enum {
  WIFI_MODE_ESPNOW,
  WIFI_MODE_OTA_STA,
  WIFI_MODE_OTA_AP
} wifi_operation_mode_t;

esp_err_t init_wifi();
esp_err_t change_wifi_mode(wifi_operation_mode_t new_mode);

#endif