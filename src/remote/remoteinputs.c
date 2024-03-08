#include "remoteinputs.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "iot_button.h"
#include "rom/gpio.h"
#include "time.h"
#include <driver/adc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

static const char *TAG = "PUBMOTE-REMOTEINPUTS";
// Configuration
#define BUTTON_PIN GPIO_NUM_0

uint8_t THROTTLE_VALUE = 128;
RemoteDataUnion remote_data;

void throttle_task(void *pvParameters) {
  // TODO - IMPLEMENT ADC CONTINUOUS READING
  remote_data.data.js_y = 0.69f;
  ESP_LOGI(TAG, "Throttle task ended");
  // terminate self
  vTaskDelete(NULL);
}

void init_throttle() {
  xTaskCreate(throttle_task, "throttle_task", 4096, NULL, 2, NULL);
}

static void button_single_click_cb(void *arg, void *usr_data) {
  ESP_LOGI(TAG, "BUTTON SINGLE CLICK");
}

static void button_double_click_cb(void *arg, void *usr_data) {
  ESP_LOGI(TAG, "BUTTON DOUBLE CLICK");
}

static void button_long_press_cb(void *arg, void *usr_data) {
  ESP_LOGI(TAG, "BUTTON LONG PRESSS");
}

void init_buttons() {
  // create gpio button
  button_config_t gpio_btn_cfg = {
      .type = BUTTON_TYPE_GPIO,
      .long_press_time = CONFIG_BUTTON_LONG_PRESS_TIME_MS,
      .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS,
      .gpio_button_config =
          {
              .gpio_num = BUTTON_PIN,
              .active_level = 0,
          },
  };
  button_handle_t gpio_btn = iot_button_create(&gpio_btn_cfg);
  if (NULL == gpio_btn) {
    ESP_LOGE(TAG, "Button create failed");
  }

  iot_button_register_cb(gpio_btn, BUTTON_SINGLE_CLICK, button_single_click_cb, NULL);
  iot_button_register_cb(gpio_btn, BUTTON_DOUBLE_CLICK, button_double_click_cb, NULL);
  iot_button_register_cb(gpio_btn, BUTTON_LONG_PRESS_UP, button_long_press_cb, NULL);
}