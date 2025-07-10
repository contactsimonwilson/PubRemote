/*
 * SPDX-FileCopyrightText: 2015-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "driver/gpio.h"
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

#define POINT_NUM_MAX 1

#define CURR_POINT_REG 0x02
#define TOUCH1_X_H_REG 0x03
#define TOUCH1_X_L_REG 0x04
#define TOUCH1_Y_H_REG 0x05
#define TOUCH1_Y_L_REG 0x06

#define FT3168_EVENT_FLAG_MASK 0xC0 // Bits 7:6
#define FT3168_COORD_MASK 0x0F      // Bits 3:0

typedef enum {
  FT3168_EVENT_PRESS = 0x00,
  FT3168_EVENT_LIFT = 0x40,
  FT3168_EVENT_CONTACT = 0x80,
  FT3168_EVENT_NONE = 0xC0
} ft3168_event_t;

static const char *TAG = "FT3168";

static esp_err_t read_data(esp_lcd_touch_handle_t tp);
static bool get_xy(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y, uint16_t *strength, uint8_t *point_num,
                   uint8_t max_point_num);
static esp_err_t del(esp_lcd_touch_handle_t tp);

static esp_err_t i2c_read_bytes(esp_lcd_touch_handle_t tp, uint16_t reg, uint8_t *data, uint8_t len);

static esp_err_t reset(esp_lcd_touch_handle_t tp);

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
  typedef struct {
    uint8_t num;
    uint8_t x_h : 4;
    uint8_t : 4;
    uint8_t x_l;
    uint8_t y_h : 4;
    uint8_t : 4;
    uint8_t y_l;
  } data_t;

  data_t point = {0};
  uint8_t tmp = 0;
  ESP_RETURN_ON_ERROR(i2c_read_bytes(tp, CURR_POINT_REG, (uint8_t *)&tmp, sizeof(tmp)), TAG,
                      "I2C read failed (touch points)");

  point.num = tmp;

  ESP_RETURN_ON_ERROR(i2c_read_bytes(tp, TOUCH1_X_H_REG, (uint8_t *)&tmp, sizeof(tmp)), TAG,
                      "I2C read failed (touch points)");
  point.x_h = tmp;

  ESP_RETURN_ON_ERROR(i2c_read_bytes(tp, TOUCH1_X_L_REG, (uint8_t *)&tmp, sizeof(tmp)), TAG,
                      "I2C read failed (touch points)");
  point.x_l = tmp;

  ESP_RETURN_ON_ERROR(i2c_read_bytes(tp, TOUCH1_Y_H_REG, (uint8_t *)&tmp, sizeof(tmp)), TAG,
                      "I2C read failed (touch points)");

  point.y_h = tmp;

  ESP_RETURN_ON_ERROR(i2c_read_bytes(tp, TOUCH1_Y_L_REG, (uint8_t *)&tmp, sizeof(tmp)), TAG,
                      "I2C read failed (touch points)");

  point.y_l = tmp;

  portENTER_CRITICAL(&tp->data.lock);
  point.num = (point.num > POINT_NUM_MAX ? POINT_NUM_MAX : point.num);
  tp->data.points = point.num;
  /* Fill all coordinates */
  for (int i = 0; i < point.num; i++) {
    tp->data.coords[i].x = ((point.x_h & FT3168_COORD_MASK) << 8) | point.x_l;
    tp->data.coords[i].y = ((point.y_h & FT3168_COORD_MASK) << 8) | point.y_l;
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
    vTaskDelay(pdMS_TO_TICKS(200));
    ESP_RETURN_ON_ERROR(gpio_set_level(tp->config.rst_gpio_num, !tp->config.levels.reset), TAG,
                        "GPIO set level failed");
    vTaskDelay(pdMS_TO_TICKS(200));
  }

  return ESP_OK;
}

static esp_err_t i2c_read_bytes(esp_lcd_touch_handle_t tp, uint16_t reg, uint8_t *data, uint8_t len) {
  ESP_RETURN_ON_FALSE(data, ESP_ERR_INVALID_ARG, TAG, "Invalid data");

  return esp_lcd_panel_io_rx_param(tp->io, reg, data, len);
}
