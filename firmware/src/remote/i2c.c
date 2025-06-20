#include "i2c.h"
#include "config.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "PUBREMOTE-I2C";

static SemaphoreHandle_t i2c_mutex = NULL;

#define I2C_MASTER_NUM I2C_NUM_0

// I2C write function with mutex protection
esp_err_t i2c_write_with_mutex(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, size_t len, int timeout_ms) {
  if (i2c_mutex == NULL) {
    ESP_LOGE(TAG, "I2C mutex not initialized");
    return ESP_FAIL;
  }

  // Take mutex with timeout
  if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(timeout_ms)) != pdTRUE) {
    ESP_LOGE(TAG, "Failed to take I2C mutex for write");
    return ESP_ERR_TIMEOUT;
  }

  esp_err_t ret = ESP_OK;
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();

  // Start condition
  i2c_master_start(cmd);

  // Write device address + write bit
  i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_WRITE, true);

  // Write register address
  i2c_master_write_byte(cmd, reg_addr, true);

  // Write data
  if (len > 0 && data != NULL) {
    i2c_master_write(cmd, data, len, true);
  }

  // Stop condition
  i2c_master_stop(cmd);

  // Execute the command
  ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(timeout_ms));

  // Clean up
  i2c_cmd_link_delete(cmd);

  // Release mutex
  xSemaphoreGive(i2c_mutex);

  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "I2C write failed: %s", esp_err_to_name(ret));
  }

  return ret;
}

// I2C read function with mutex protection
esp_err_t i2c_read_with_mutex(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, size_t len, int timeout_ms) {
  if (i2c_mutex == NULL) {
    ESP_LOGE(TAG, "I2C mutex not initialized");
    return ESP_FAIL;
  }

  if (data == NULL || len == 0) {
    ESP_LOGE(TAG, "Invalid read parameters");
    return ESP_ERR_INVALID_ARG;
  }

  // Take mutex with timeout
  if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(timeout_ms)) != pdTRUE) {
    ESP_LOGE(TAG, "Failed to take I2C mutex for read");
    return ESP_ERR_TIMEOUT;
  }

  esp_err_t ret = ESP_OK;
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();

  // Start condition
  i2c_master_start(cmd);

  // Write device address + write bit to set register address
  i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_WRITE, true);

  // Write register address
  i2c_master_write_byte(cmd, reg_addr, true);

  // Repeated start condition
  i2c_master_start(cmd);

  // Write device address + read bit
  i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_READ, true);

  // Read data
  if (len > 1) {
    i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
  }
  i2c_master_read_byte(cmd, &data[len - 1], I2C_MASTER_NACK);

  // Stop condition
  i2c_master_stop(cmd);

  // Execute the command
  ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(timeout_ms));

  // Clean up
  i2c_cmd_link_delete(cmd);

  // Release mutex
  xSemaphoreGive(i2c_mutex);

  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "I2C read failed: %s", esp_err_to_name(ret));
  }

  return ret;
}

// i2c_mutex_lock
bool i2c_lock(int timeout_ms) {
  if (i2c_mutex == NULL) {
    ESP_LOGE(TAG, "I2C not initialized");
    return false;
  }

  if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(timeout_ms)) == pdTRUE) {
    return true;
  }
  else {
    ESP_LOGE(TAG, "Failed to acquire I2C mutex");
    return false;
  }
}

bool i2c_unlock() {
  if (i2c_mutex == NULL) {
    ESP_LOGE(TAG, "I2C not initialized");
    return false;
  }

  if (xSemaphoreGive(i2c_mutex) == pdTRUE) {
    return true;
  }
  else {
    ESP_LOGE(TAG, "Failed to release I2C mutex");
    return false;
  }
}

void init_i2c() {
  // Create a mutex for I2C operations
  i2c_mutex = xSemaphoreCreateMutex();

  /* Initilize I2C */
  const i2c_config_t i2c_conf = {.mode = I2C_MODE_MASTER,
                                 .sda_io_num = I2C_SDA,
                                 .sda_pullup_en = GPIO_PULLUP_DISABLE,
                                 .scl_io_num = I2C_SCL,
                                 .scl_pullup_en = GPIO_PULLUP_DISABLE,
                                 .master.clk_speed = 400000};

  ESP_LOGI(TAG, "Initializing I2C for display touch");
  /* Initialize I2C */
  ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &i2c_conf));
  ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, i2c_conf.mode, 0, 0, 0));
}