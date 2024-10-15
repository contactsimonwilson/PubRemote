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

void save_calibration();

typedef enum {
  AUTO_OFF_DISABLED,
  AUTO_OFF_2_MINUTES,
  AUTO_OFF_5_MINUTES,
  AUTO_OFF_10_MINUTES,
} AutoOffOptions;

typedef struct {
  uint16_t x_min;
  uint16_t x_max;
  uint16_t y_min;
  uint16_t y_max;
  uint16_t x_center;
  uint16_t y_center;
  uint16_t deadband;
  float expo;
} CalibrationSettings;

typedef struct {
  uint8_t bl_level;
  AutoOffOptions auto_off_time;
} DeviceSettings;

uint64_t get_auto_off_ms();

extern CalibrationSettings calibration_settings;
extern DeviceSettings device_settings;

#endif