#include <core/lv_obj.h>
#ifndef __SETTINGS_H
  #define __SETTINGS_H

  #include "esp_system.h"
  #include "nvs_flash.h"

// Function to initialize NVS
esp_err_t init_nvs();

// Function to initialize settings (and NVS)
esp_err_t init_settings();

// Function to write an integer to NVS
esp_err_t nvs_write_int(const char *key, int32_t value);

// Function to read an integer from NVS
esp_err_t nvs_read_int(const char *key, int32_t *value);

void save_bl_level();

void save_auto_off_time();

typedef enum {
  AUTO_OFF_DISABLED,
  AUTO_OFF_2_MINUTES,
  AUTO_OFF_5_MINUTES,
  AUTO_OFF_10_MINUTES,
} AutoOffOptions;

typedef struct {
  int16_t x_min;
  int16_t x_max;
  int16_t y_min;
  int16_t y_max;
  int16_t x_center;
  int16_t y_center;
  int16_t deadzone;
} StickCalibration;

typedef struct {
  uint8_t bl_level;
  AutoOffOptions auto_off_time;
  StickCalibration stick_calibration;
} RemoteSettings;

uint64_t get_auto_off_ms();

extern RemoteSettings settings;

#endif