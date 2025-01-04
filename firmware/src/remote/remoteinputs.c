#include "remoteinputs.h"
#include "adc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "iot_button.h"
#include "powermanagement.h"
#include "rom/gpio.h"
#include "settings.h"
#include "time.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <math.h>
#include <ui/ui.h>

static const char *TAG = "PUBREMOTE-REMOTEINPUTS";
#define SLEEP_DISRUPT_THRESHOLD 25

// TODO - from SETTINGS
#ifndef JOYSTICK_BUTTON_LEVEL
  #error "JOYSTICK_BUTTON_LEVEL must be defined"
#endif

RemoteDataUnion remote_data;
JoystickData joystick_data;
static button_handle_t gpio_btn_handle = NULL;
static bool power_button_enabled = true;

float convert_adc_to_axis(int adc_value, int min_val, int mid_val, int max_val, int deadband, float expo) {
  float axis = 0;

  int mid_val_lower = mid_val - deadband;
  int mid_val_upper = mid_val + deadband;

  if (adc_value > mid_val_lower && adc_value < mid_val_upper) {
    // Within deadband
    return 0;
  }

  // Apply across adjusted mid vals so we get smooth ramping outside of deadband
  if (adc_value > mid_val) {
    axis = (float)(adc_value - mid_val_upper) / (max_val - mid_val_upper);
  }
  else {
    axis = (float)(adc_value - mid_val_lower) / (mid_val_lower - min_val);
  }

  // Apply expo
  if (expo > 1) {
    bool negative = axis < 0;
    axis = pow(axis, expo);
    if (negative) {
      axis = -axis;
    }
  }

  // clamp between -1 and 1
  if (axis > 1) {
    axis = 1;
  }
  else if (axis < -1) {
    axis = -1;
  }

  // Round to 2 decimal places
  axis = roundf(axis * 100) / 100;

  return axis;
}

static void thumbstick_task(void *pvParameters) {
#if (JOYSTICK_Y_ENABLED || JOYSTICK_X_ENABLED)
  #if JOYSTICK_X_ENABLED
    #if JOYSTICK_X_ADC_UNIT == 1 // ADC_UNIT_1
  adc_oneshot_unit_handle_t x_adc_handle = adc1_handle;
    #elif JOYSTICK_X_ADC_UNIT == 2 // ADC_UNIT_2
  adc_oneshot_unit_handle_t x_adc_handle = adc2_handle;
    #else
      #error "Invalid ADC unit"
    #endif
  ESP_ERROR_CHECK(adc_oneshot_config_channel(x_adc_handle, JOYSTICK_X_ADC, &adc_channel_config));
  #endif

  #if JOYSTICK_Y_ENABLED
    #if JOYSTICK_Y_ADC_UNIT == 1 // ADC_UNIT_1
  adc_oneshot_unit_handle_t y_adc_handle = adc1_handle;
    #elif JOYSTICK_Y_ADC_UNIT == 2 // ADC_UNIT_2
  adc_oneshot_unit_handle_t y_adc_handle = adc2_handle;
    #else
      #error "Invalid ADC unit"
    #endif

  ESP_ERROR_CHECK(adc_oneshot_config_channel(y_adc_handle, JOYSTICK_Y_ADC, &adc_channel_config));
  #endif

#endif

  while (1) {
    bool trigger_sleep_disrupt = false;
    int16_t deadband = calibration_settings.deadband;
    int16_t x_center = calibration_settings.x_center;
    int16_t y_center = calibration_settings.y_center;
    int16_t y_max = calibration_settings.y_max;
    int16_t x_max = calibration_settings.x_max;
    int16_t y_min = calibration_settings.y_min;
    int16_t x_min = calibration_settings.x_min;
    float expo = calibration_settings.expo;

#if JOYSTICK_X_ENABLED
    int x_value;
    ESP_ERROR_CHECK(adc_oneshot_read(x_adc_handle, JOYSTICK_X_ADC, &x_value));
    joystick_data.x = x_value;
    float new_x = convert_adc_to_axis(x_value, x_min, x_center, x_max, deadband, expo);

    if (new_x != remote_data.data.js_x) {
      remote_data.data.js_x = new_x;
      trigger_sleep_disrupt = true;
    }
#endif

#if JOYSTICK_Y_ENABLED
    int y_value;
    ESP_ERROR_CHECK(adc_oneshot_read(y_adc_handle, JOYSTICK_Y_ADC, &y_value));
    joystick_data.y = y_value;
    float new_y = convert_adc_to_axis(y_value, y_min, y_center, y_max, deadband, expo);

    if (new_y != remote_data.data.js_y) {
      remote_data.data.js_y = new_y;
      trigger_sleep_disrupt = true;
    }
#endif

    if (trigger_sleep_disrupt) {
      reset_sleep_timer();
    }

    vTaskDelay(pdMS_TO_TICKS(INPUT_RATE_MS));
  }

  ESP_LOGI(TAG, "Thumbstick task ended");
  vTaskDelete(NULL);
}

void init_thumbstick() {
#if (JOYSTICK_Y_ENABLED || JOYSTICK_X_ENABLED)
  xTaskCreatePinnedToCore(thumbstick_task, "thumbstick_task", 4096, NULL, 20, NULL, 0);
#endif
}

static void button_single_click_cb(void *arg, void *usr_data) {
  ESP_LOGI(TAG, "BUTTON SINGLE CLICK");
  reset_sleep_timer();
  remote_data.data.bt_c = 1;

  // Start a timer to reset the button state after a certain duration
  esp_timer_handle_t reset_timer;
  esp_timer_create_args_t timer_args = {.callback = reset_button_state, .name = "reset_button_timer"};
  ESP_ERROR_CHECK(esp_timer_create(&timer_args, &reset_timer));
  ESP_ERROR_CHECK(esp_timer_start_once(reset_timer, 1000000)); // 1000ms delay
}

static void button_double_click_cb(void *arg, void *usr_data) {
  ESP_LOGI(TAG, "BUTTON DOUBLE CLICK");
  reset_sleep_timer();
}

static void button_long_press_cb(void *arg, void *usr_data) {
  ESP_LOGI(TAG, "BUTTON LONG PRESS");
  if (power_button_enabled) {
    enter_sleep();
  }
}

void reset_button_state() {
  remote_data.data.bt_c = 0;
}

void init_buttons() {
#if JOYSTICK_BUTTON_ENABLED
  // create gpio button
  button_config_t gpio_btn_cfg = {
      .type = BUTTON_TYPE_GPIO,
      .long_press_time = CONFIG_BUTTON_LONG_PRESS_TIME_MS,
      .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS,
      .gpio_button_config =
          {
              .gpio_num = JOYSTICK_BUTTON_PIN,
              .active_level = JOYSTICK_BUTTON_LEVEL,
          },
  };
  gpio_btn_handle = iot_button_create(&gpio_btn_cfg);
  if (NULL == gpio_btn_handle) {
    ESP_LOGE(TAG, "Button create failed");
  }

  iot_button_register_cb(gpio_btn_handle, BUTTON_SINGLE_CLICK, button_single_click_cb, NULL);
  iot_button_register_cb(gpio_btn_handle, BUTTON_DOUBLE_CLICK, button_double_click_cb, NULL);
  iot_button_register_cb(gpio_btn_handle, BUTTON_LONG_PRESS_UP, button_long_press_cb, NULL);
#endif
}

void enable_power_button(bool enable) {
  power_button_enabled = enable;
}