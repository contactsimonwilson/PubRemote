#include "settings.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "remote/adc.h"
#include <stdio.h>

static const char *TAG = "PUBREMOTE-SETTINGS";

// Define the NVS namespace
#define STORAGE_NAMESPACE "nvs"
#define BL_LEVEL_KEY "bl_level"
#define BL_LEVEL_DEFAULT 200
#define AUTO_OFF_TIME_KEY "auto_off_time"
#define EXPO_ADJUST_FACTOR 100

static const AutoOffOptions DEFAULT_AUTO_OFF_TIME = AUTO_OFF_5_MINUTES;

RemoteSettings settings = {
    .bl_level = BL_LEVEL_DEFAULT,
    .auto_off_time = DEFAULT_AUTO_OFF_TIME,
    .stick_calibration =
        {
            .x_min = STICK_MIN_VAL,
            .x_max = STICK_MAX_VAL,
            .y_min = STICK_MIN_VAL,
            .y_max = STICK_MAX_VAL,
            .x_center = STICK_MID_VAL,
            .y_center = STICK_MID_VAL,
            .deadband = STICK_DEADBAND,
            .expo = STICK_EXPO,
        },
};

static uint8_t get_auto_off_time_minutes() {
  switch (settings.auto_off_time) {
  case AUTO_OFF_DISABLED:
    return 0;
  case AUTO_OFF_2_MINUTES:
    return 2;
  case AUTO_OFF_5_MINUTES:
    return 5;
  case AUTO_OFF_10_MINUTES:
    return 10;
  default:
    return 0;
  }
}

uint64_t get_auto_off_ms() {
  return get_auto_off_time_minutes() * 60 * 1000;
}

void save_bl_level() {
  nvs_write_int(BL_LEVEL_KEY, settings.bl_level);
}

void save_auto_off_time() {
  nvs_write_int(AUTO_OFF_TIME_KEY, settings.auto_off_time);
}

void save_calibration() {
  nvs_write_int("x_min", settings.stick_calibration.x_min);
  nvs_write_int("x_max", settings.stick_calibration.x_max);
  nvs_write_int("y_min", settings.stick_calibration.y_min);
  nvs_write_int("y_max", settings.stick_calibration.y_max);
  nvs_write_int("x_center", settings.stick_calibration.x_center);
  nvs_write_int("y_center", settings.stick_calibration.y_center);
  nvs_write_int("deadband", settings.stick_calibration.deadband);
  nvs_write_int("expo", (int)(settings.stick_calibration.expo * EXPO_ADJUST_FACTOR));
}

// Function to initialize NVS
esp_err_t init_nvs() {
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    // NVS partition was truncated and needs to be erased
    // Retry nvs_flash_init
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);
  return ESP_OK;
}

// Function to initialize settings object, reading from NVS
esp_err_t init_settings() {
  esp_err_t err = init_nvs();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error initializing NVS!");
    return err;
  }

  settings.bl_level = nvs_read_int("bl_level", &settings.bl_level) == ESP_OK ? settings.bl_level : BL_LEVEL_DEFAULT;
  settings.auto_off_time =
      nvs_read_int("auto_off_time", &settings.auto_off_time) == ESP_OK ? settings.auto_off_time : DEFAULT_AUTO_OFF_TIME;

  settings.stick_calibration.x_min = nvs_read_int("x_min", &settings.stick_calibration.x_min) == ESP_OK
                                         ? settings.stick_calibration.x_min
                                         : STICK_MIN_VAL;
  settings.stick_calibration.x_max = nvs_read_int("x_max", &settings.stick_calibration.x_max) == ESP_OK
                                         ? settings.stick_calibration.x_max
                                         : STICK_MAX_VAL;

  settings.stick_calibration.y_min = nvs_read_int("y_min", &settings.stick_calibration.y_min) == ESP_OK
                                         ? settings.stick_calibration.y_min
                                         : STICK_MIN_VAL;

  settings.stick_calibration.y_max = nvs_read_int("y_max", &settings.stick_calibration.y_max) == ESP_OK
                                         ? settings.stick_calibration.y_max
                                         : STICK_MAX_VAL;

  settings.stick_calibration.x_center = nvs_read_int("x_center", &settings.stick_calibration.x_center) == ESP_OK
                                            ? settings.stick_calibration.x_center
                                            : STICK_MID_VAL;

  settings.stick_calibration.y_center = nvs_read_int("y_center", &settings.stick_calibration.y_center) == ESP_OK
                                            ? settings.stick_calibration.y_center
                                            : STICK_MID_VAL;

  settings.stick_calibration.deadband = nvs_read_int("deadband", &settings.stick_calibration.deadband) == ESP_OK
                                            ? settings.stick_calibration.deadband
                                            : STICK_DEADBAND;

  int16_t expo = STICK_EXPO;

  settings.stick_calibration.expo =
      nvs_read_int("x_expo", &expo) == ESP_OK ? (float)(expo / EXPO_ADJUST_FACTOR) : STICK_EXPO;

  return ESP_OK;
}

// Function to write an integer to NVS
esp_err_t nvs_write_int(const char *key, int32_t value) {
  nvs_handle_t my_handle;
  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    return err;
  }

  err = nvs_set_i32(my_handle, key, value);

  if (err != ESP_OK) {
    ESP_LOGI(TAG, "Write done");
  }
  else {
    ESP_LOGE(TAG, "Failed to write!");
  }

  err = nvs_commit(my_handle);
  if (err != ESP_OK) {
    ESP_LOGI(TAG, "Commit done");
  }
  else {
    ESP_LOGE(TAG, "Failed to commit!");
  }

  nvs_close(my_handle);
  return err;
}

// Function to read an integer from NVS
esp_err_t nvs_read_int(const char *key, int32_t *value) {
  nvs_handle_t my_handle;
  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &my_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    return err;
  }

  err = nvs_get_i32(my_handle, key, value);
  switch (err) {
  case ESP_OK:
    ESP_LOGI(TAG, "Read done, value = %ld\n", *value);
    break;
  case ESP_ERR_NVS_NOT_FOUND:
    ESP_LOGE(TAG, "The value is not initialized yet!");
    break;
  default:
    ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));
  }

  nvs_close(my_handle);
  return err;
}