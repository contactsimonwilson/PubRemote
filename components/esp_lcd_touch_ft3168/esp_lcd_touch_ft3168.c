/*
 * SPDX-FileCopyrightText: 2015-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define POINT_NUM_MAX (5)

#define FT3168_I2C_ADDR (0x38)
#define REG_MODE (0x00)
#define REG_GEST_ID (0x01)
#define REG_TD_STATUS (0x02)
#define REG_TOUCH_DATA (0x03)
#define REG_CHIP_ID (0xA3)

static const char *TAG = "FT3168";

static esp_err_t read_data(esp_lcd_touch_handle_t tp);
static bool get_xy(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y, uint16_t *strength, uint8_t *point_num,
                   uint8_t max_point_num);
static esp_err_t del(esp_lcd_touch_handle_t tp);

static esp_err_t i2c_read_bytes(esp_lcd_touch_handle_t tp, uint16_t reg, uint8_t *data, uint8_t len);
static esp_err_t i2c_write_byte(esp_lcd_touch_handle_t tp, uint16_t reg, uint8_t data);

static esp_err_t reset(esp_lcd_touch_handle_t tp);
static esp_err_t read_id(esp_lcd_touch_handle_t tp);

esp_err_t esp_lcd_touch_new_i2c_ft3168(const esp_lcd_panel_io_handle_t io, const esp_lcd_touch_config_t *config,
                                       esp_lcd_touch_handle_t *tp) {
  ESP_RETURN_ON_FALSE(io, ESP_ERR_INVALID_ARG, TAG, "Invalid io");
  ESP_RETURN_ON_FALSE(config, ESP_ERR_INVALID_ARG, TAG, "Invalid config");
  ESP_RETURN_ON_FALSE(tp, ESP_ERR_INVALID_ARG, TAG, "Invalid touch handle");

  /* Prepare main structure */
  esp_err_t ret = ESP_OK;
  esp_lcd_touch_handle_t ft3168 = calloc(1, sizeof(esp_lcd_touch_t));
  ESP_GOTO_ON_FALSE(ft3168, ESP_ERR_NO_MEM, err, TAG, "Touch handle malloc failed");

  /* Communication interface */
  ft3168->io = io;
  /* Only supported callbacks are set */
  ft3168->read_data = read_data;
  ft3168->get_xy = get_xy;
  ft3168->del = del;
  /* Mutex */
  ft3168->data.lock.owner = portMUX_FREE_VAL;
  /* Save config */
  memcpy(&ft3168->config, config, sizeof(esp_lcd_touch_config_t));

  /* Prepare pin for touch interrupt */
  if (ft3168->config.int_gpio_num != GPIO_NUM_NC) {
    const gpio_config_t int_gpio_config = {
        .mode = GPIO_MODE_INPUT,
        .intr_type = (ft3168->config.levels.interrupt ? GPIO_INTR_POSEDGE : GPIO_INTR_NEGEDGE),
        .pin_bit_mask = BIT64(ft3168->config.int_gpio_num)};
    ESP_GOTO_ON_ERROR(gpio_config(&int_gpio_config), err, TAG, "GPIO intr config failed");

    /* Register interrupt callback */
    if (ft3168->config.interrupt_callback) {
      esp_lcd_touch_register_interrupt_callback(ft3168, ft3168->config.interrupt_callback);
    }
  }
  /* Prepare pin for touch controller reset */
  if (ft3168->config.rst_gpio_num != GPIO_NUM_NC) {
    const gpio_config_t rst_gpio_config = {.mode = GPIO_MODE_OUTPUT,
                                           .pin_bit_mask = BIT64(ft3168->config.rst_gpio_num)};
    ESP_GOTO_ON_ERROR(gpio_config(&rst_gpio_config), err, TAG, "GPIO reset config failed");
  }
  /* Reset controller */
  ESP_GOTO_ON_ERROR(reset(ft3168), err, TAG, "Reset failed");
  /* Read product id */
  ESP_GOTO_ON_ERROR(read_id(ft3168), err, TAG, "Read version failed");
  *tp = ft3168;

  return ESP_OK;
err:
  if (ft3168) {
    del(ft3168);
  }
  ESP_LOGE(TAG, "Initialization failed!");
  return ret;
}

static esp_err_t read_data(esp_lcd_touch_handle_t tp) {
  uint8_t data[4 * POINT_NUM_MAX];
  ESP_RETURN_ON_ERROR(i2c_read_bytes(tp, REG_TOUCH_DATA, data, sizeof(data)), TAG, "I2C read failed");

  uint8_t touch_points = data[REG_TD_STATUS - REG_TOUCH_DATA] & 0x0F;

  portENTER_CRITICAL(&tp->data.lock);
  tp->data.points = (touch_points > POINT_NUM_MAX ? POINT_NUM_MAX : touch_points);
  for (int i = 0; i < tp->data.points; i++) {
    tp->data.coords[i].x = ((data[i * 6 + 0] & 0x0F) << 8) | data[i * 6 + 1];
    tp->data.coords[i].y = ((data[i * 6 + 2] & 0x0F) << 8) | data[i * 6 + 3];
  }
  portEXIT_CRITICAL(&tp->data.lock);

  return ESP_OK;
}

static bool get_xy(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y, uint16_t *strength, uint8_t *point_num,
                   uint8_t max_point_num) {
  portENTER_CRITICAL(&tp->data.lock);
  /* Count of points */
  *point_num = (tp->data.points > max_point_num ? max_point_num : tp->data.points);
  for (size_t i = 0; i < *point_num; i++) {
    x[i] = tp->data.coords[i].x;
    y[i] = tp->data.coords[i].y;

    if (strength) {
      strength[i] = tp->data.coords[i].strength;
    }
  }
  /* Invalidate */
  tp->data.points = 0;
  portEXIT_CRITICAL(&tp->data.lock);

  return (*point_num > 0);
}

static esp_err_t del(esp_lcd_touch_handle_t tp) {
  /* Reset GPIO pin settings */
  if (tp->config.int_gpio_num != GPIO_NUM_NC) {
    gpio_reset_pin(tp->config.int_gpio_num);
    if (tp->config.interrupt_callback) {
      gpio_isr_handler_remove(tp->config.int_gpio_num);
    }
  }
  if (tp->config.rst_gpio_num != GPIO_NUM_NC) {
    gpio_reset_pin(tp->config.rst_gpio_num);
  }
  /* Release memory */
  free(tp);

  return ESP_OK;
}

static esp_err_t reset(esp_lcd_touch_handle_t tp) {
  if (tp->config.rst_gpio_num != GPIO_NUM_NC) {
    ESP_RETURN_ON_ERROR(gpio_set_level(tp->config.rst_gpio_num, tp->config.levels.reset), TAG, "GPIO set level failed");
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_RETURN_ON_ERROR(gpio_set_level(tp->config.rst_gpio_num, !tp->config.levels.reset), TAG,
                        "GPIO set level failed");
    vTaskDelay(pdMS_TO_TICKS(50));
  }

  // Set to normal operation mode
  ESP_RETURN_ON_ERROR(i2c_write_byte(tp, REG_MODE, 0x00), TAG, "Set mode failed");

  return ESP_OK;
}

static esp_err_t read_id(esp_lcd_touch_handle_t tp) {
  uint8_t id;
  ESP_RETURN_ON_ERROR(i2c_read_bytes(tp, REG_CHIP_ID, &id, 1), TAG, "I2C read failed");
  ESP_LOGI(TAG, "IC id: 0x%02X", id);
  return ESP_OK;
}

static esp_err_t i2c_read_bytes(esp_lcd_touch_handle_t tp, uint16_t reg, uint8_t *data, uint8_t len) {
  ESP_RETURN_ON_FALSE(data, ESP_ERR_INVALID_ARG, TAG, "Invalid data");
  return esp_lcd_panel_io_rx_param(tp->io, reg, data, len);
}

static esp_err_t i2c_write_byte(esp_lcd_touch_handle_t tp, uint16_t reg, uint8_t data) {
  return esp_lcd_panel_io_tx_param(tp->io, reg, &data, 1);
}