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
#define EXPO_ADJUST_FACTOR 100 // Stored as 2dp int
#define THEME_COLOR_DEFAULT 0x2095f6

static const AutoOffOptions DEFAULT_AUTO_OFF_TIME = AUTO_OFF_5_MINUTES;
static const uint8_t DEFAULT_PEER_ADDR[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

DeviceSettings device_settings = {
    .bl_level = BL_LEVEL_DEFAULT,
    .auto_off_time = DEFAULT_AUTO_OFF_TIME,
    .temp_units = TEMP_UNITS_CELSIUS,
    .distance_units = DISTANCE_UNITS_METRIC,
    .theme_color = THEME_COLOR_DEFAULT,
};

CalibrationSettings calibration_settings = {
    .x_min = STICK_MIN_VAL,
    .x_max = STICK_MAX_VAL,
    .y_min = STICK_MIN_VAL,
    .y_max = STICK_MAX_VAL,
    .x_center = STICK_MID_VAL,
    .y_center = STICK_MID_VAL,
    .deadband = STICK_DEADBAND,
    .expo = STICK_EXPO,

};

PairingSettings pairing_settings = {
    .state = PAIRING_STATE_UNPAIRED,
    .remote_addr = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // Use 0xFF for -1 as uint8_t is unsigned
    .secret_code = -1};

static uint8_t get_auto_off_time_minutes() {
  switch (device_settings.auto_off_time) {
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

void save_device_settings() {
  nvs_write_int(BL_LEVEL_KEY, device_settings.bl_level);
  nvs_write_int(AUTO_OFF_TIME_KEY, device_settings.auto_off_time);
  nvs_write_int("temp_units", device_settings.temp_units);
  nvs_write_int("distance_units", device_settings.distance_units);
  nvs_write_int("theme_color", device_settings.theme_color);
}

esp_err_t save_pairing_data() {
  ESP_LOGI(TAG, "Saving pairing data...");
  esp_err_t err = nvs_write_int("pairing_state", (int32_t)pairing_settings.state);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error saving pairing state!");
    return err;
  }
  err = nvs_write_int("secret_code", pairing_settings.secret_code);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error saving secret code!");
    return err;
  }

  err = nvs_write_blob("remote_addr", pairing_settings.remote_addr, sizeof(pairing_settings.remote_addr));
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error saving remote address!");
    return err;
  }

  ESP_LOGI(TAG, "Pairing data saved successfully.");
  return ESP_OK;
}

void save_calibration() {
  nvs_write_int("x_min", calibration_settings.x_min);
  nvs_write_int("x_max", calibration_settings.x_max);
  nvs_write_int("y_min", calibration_settings.y_min);
  nvs_write_int("y_max", calibration_settings.y_max);
  nvs_write_int("x_center", calibration_settings.x_center);
  nvs_write_int("y_center", calibration_settings.y_center);
  nvs_write_int("deadband", calibration_settings.deadband);
  nvs_write_int("expo", (int)(calibration_settings.expo * EXPO_ADJUST_FACTOR));
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
  ESP_LOGI(TAG, "Initializing settings...");
  esp_err_t err = init_nvs();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error initializing NVS!");
    return err;
  }

  // Reading device settings
  device_settings.bl_level =
      nvs_read_int("bl_level", &device_settings.bl_level) == ESP_OK ? device_settings.bl_level : BL_LEVEL_DEFAULT;
  device_settings.auto_off_time = nvs_read_int("auto_off_time", &device_settings.auto_off_time) == ESP_OK
                                      ? device_settings.auto_off_time
                                      : DEFAULT_AUTO_OFF_TIME;

  device_settings.temp_units = nvs_read_int("temp_units", &device_settings.temp_units) == ESP_OK
                                   ? device_settings.temp_units
                                   : TEMP_UNITS_CELSIUS;

  device_settings.distance_units = nvs_read_int("distance_units", &device_settings.distance_units) == ESP_OK
                                       ? device_settings.distance_units
                                       : DISTANCE_UNITS_METRIC;

  device_settings.theme_color = nvs_read_int("theme_color", &device_settings.theme_color) == ESP_OK
                                    ? device_settings.theme_color
                                    : THEME_COLOR_DEFAULT;

  // Reading calibration settings
  calibration_settings.x_min =
      nvs_read_int("x_min", &calibration_settings.x_min) == ESP_OK ? calibration_settings.x_min : STICK_MIN_VAL;
  calibration_settings.x_max =
      nvs_read_int("x_max", &calibration_settings.x_max) == ESP_OK ? calibration_settings.x_max : STICK_MAX_VAL;

  calibration_settings.y_min =
      nvs_read_int("y_min", &calibration_settings.y_min) == ESP_OK ? calibration_settings.y_min : STICK_MIN_VAL;

  calibration_settings.y_max =
      nvs_read_int("y_max", &calibration_settings.y_max) == ESP_OK ? calibration_settings.y_max : STICK_MAX_VAL;

  calibration_settings.x_center = nvs_read_int("x_center", &calibration_settings.x_center) == ESP_OK
                                      ? calibration_settings.x_center
                                      : STICK_MID_VAL;

  calibration_settings.y_center = nvs_read_int("y_center", &calibration_settings.y_center) == ESP_OK
                                      ? calibration_settings.y_center
                                      : STICK_MID_VAL;

  calibration_settings.deadband = nvs_read_int("deadband", &calibration_settings.deadband) == ESP_OK
                                      ? calibration_settings.deadband
                                      : STICK_DEADBAND;

  int16_t expo = STICK_EXPO;

  calibration_settings.expo = nvs_read_int("x_expo", &expo) == ESP_OK ? (float)(expo / EXPO_ADJUST_FACTOR) : STICK_EXPO;

  // Reading pairing settings
  pairing_settings.state = nvs_read_int("pairing_state", &pairing_settings.state) == ESP_OK ? pairing_settings.state
                                                                                            : PAIRING_STATE_UNPAIRED;

  pairing_settings.secret_code =
      nvs_read_int("secret_code", &pairing_settings.secret_code) == ESP_OK ? pairing_settings.secret_code : -1;

  uint8_t remote_addr[6];
  err = nvs_read_blob("remote_addr", &remote_addr, sizeof(remote_addr));
  if (err == ESP_OK) {
    memcpy(pairing_settings.remote_addr, remote_addr, sizeof(remote_addr));
  }
  else {
    memcpy(pairing_settings.remote_addr, DEFAULT_PEER_ADDR, sizeof(DEFAULT_PEER_ADDR));
  }

  return ESP_OK;
}

static esp_err_t nvs_write(const char *key, void *value, nvs_type_t type, size_t length) {
  nvs_handle_t nvs_handle;
  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    return err;
  }

  switch (type) {
  case NVS_TYPE_I8:
    err = nvs_set_i8(nvs_handle, key, *(int8_t *)value);
    break;
  case NVS_TYPE_U8:
    err = nvs_set_u8(nvs_handle, key, *(uint8_t *)value);
    break;
  case NVS_TYPE_I16:
    err = nvs_set_i16(nvs_handle, key, *(int16_t *)value);
    break;
  case NVS_TYPE_U16:
    err = nvs_set_u16(nvs_handle, key, *(uint16_t *)value);
    break;
  case NVS_TYPE_I32:
    err = nvs_set_i32(nvs_handle, key, *(int32_t *)value);
    break;
  case NVS_TYPE_U32:
    err = nvs_set_u32(nvs_handle, key, *(uint32_t *)value);
    break;
  case NVS_TYPE_I64:
    err = nvs_set_i64(nvs_handle, key, *(int64_t *)value);
    break;
  case NVS_TYPE_U64:
    err = nvs_set_u64(nvs_handle, key, *(uint64_t *)value);
    break;
  case NVS_TYPE_STR:
    err = nvs_set_str(nvs_handle, key, (char *)value);
    break;
  case NVS_TYPE_BLOB:
    err = nvs_set_blob(nvs_handle, key, value, length);
    break;
  default:
    ESP_LOGE(TAG, "Unsupported data type!");
    err = ESP_ERR_INVALID_ARG;
  }

  if (err != ESP_OK) {
    ESP_LOGI(TAG, "Write done");
  }
  else {
    ESP_LOGE(TAG, "Failed to write!");
  }

  err = nvs_commit(nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGI(TAG, "Commit done");
  }
  else {
    ESP_LOGE(TAG, "Failed to commit!");
  }

  nvs_close(nvs_handle);
  return err;
}

// Function to read from NVS
static esp_err_t nvs_read(const char *key, void *value, nvs_type_t type, size_t length) {
  nvs_handle_t nvs_handle;
  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    return err;
  }

  switch (type) {
  case NVS_TYPE_I8:
    err = nvs_get_i8(nvs_handle, key, (int8_t *)value);
    if (err == ESP_OK) {
      ESP_LOGI(TAG, "Read done, value = %d", *(int8_t *)value);
    }
    break;
  case NVS_TYPE_U8:
    err = nvs_get_u8(nvs_handle, key, (uint8_t *)value);
    if (err == ESP_OK) {
      ESP_LOGI(TAG, "Read done, value = %u", *(uint8_t *)value);
    }
    break;
  case NVS_TYPE_I16:
    err = nvs_get_i16(nvs_handle, key, (int16_t *)value);
    if (err == ESP_OK) {
      ESP_LOGI(TAG, "Read done, value = %d", *(int16_t *)value);
    }
    break;
  case NVS_TYPE_U16:
    err = nvs_get_u16(nvs_handle, key, (uint16_t *)value);
    if (err == ESP_OK) {
      ESP_LOGI(TAG, "Read done, value = %u", *(uint16_t *)value);
    }
    break;
  case NVS_TYPE_I32:
    err = nvs_get_i32(nvs_handle, key, (int32_t *)value);
    if (err == ESP_OK) {
      ESP_LOGI(TAG, "Read done, value = %ld", *(int32_t *)value);
    }
    break;
  case NVS_TYPE_U32:
    err = nvs_get_u32(nvs_handle, key, (uint32_t *)value);
    if (err == ESP_OK) {
      ESP_LOGI(TAG, "Read done, value = %lu", *(uint32_t *)value);
    }
    break;
  case NVS_TYPE_I64:
    err = nvs_get_i64(nvs_handle, key, (int64_t *)value);
    if (err == ESP_OK) {
      ESP_LOGI(TAG, "Read done, value = %lld", *(int64_t *)value);
    }
    break;
  case NVS_TYPE_U64:
    err = nvs_get_u64(nvs_handle, key, (uint64_t *)value);
    if (err == ESP_OK) {
      ESP_LOGI(TAG, "Read done, value = %llu", *(uint64_t *)value);
    }
    break;
  case NVS_TYPE_STR: {
    size_t required_size;
    // Get the size of the string first
    err = nvs_get_str(nvs_handle, key, NULL, &required_size);
    if (err == ESP_OK) {
      char *str_value = (char *)malloc(required_size);
      if (str_value == NULL) {
        ESP_LOGE(TAG, "Memory allocation failed!");
        err = ESP_ERR_NO_MEM;
      }
      else {
        err = nvs_get_str(nvs_handle, key, str_value, &required_size);
        if (err == ESP_OK) {
          strcpy((char *)value, str_value);
          ESP_LOGI(TAG, "Read done, value = %s", str_value);
        }
        free(str_value);
      }
    }
    break;
  }
  case NVS_TYPE_BLOB:
    err = nvs_get_blob(nvs_handle, key, value, &length);
    break;
  default:
    ESP_LOGE(TAG, "Unsupported data type!");
    err = ESP_ERR_INVALID_ARG;
  }

  switch (err) {
  case ESP_OK:
    ESP_LOGI(TAG, "Read done");
    break;
  case ESP_ERR_NVS_NOT_FOUND:
    ESP_LOGE(TAG, "The value is not initialized yet!");
    break;
  default:
    ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));
  }

  nvs_close(nvs_handle);
  return err;
}

// Function to write an integer to NVS
esp_err_t nvs_write_int(const char *key, uint32_t value) {
  return nvs_write(key, &value, NVS_TYPE_U32, 0);
}

// Function to write a blob to NVS
esp_err_t nvs_write_blob(const char *key, void *value, size_t length) {
  return nvs_write(key, &value, NVS_TYPE_BLOB, length);
}

// Function to read an integer from NVS
esp_err_t nvs_read_int(const char *key, uint32_t *value) {
  return nvs_read(key, value, NVS_TYPE_U32, 0);
}

// Function to read a blob from NVS
esp_err_t nvs_read_blob(const char *key, void *value, size_t length) {
  return nvs_read(key, value, NVS_TYPE_BLOB, length);
}
