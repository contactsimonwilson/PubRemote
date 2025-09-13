#include "remoteinputs.h"
#include "adc.h"
#include "config.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "iot_button.h"
#include "powermanagement.h"
#include "rom/gpio.h"
#include "screens/stats_screen.h"
#include "settings.h"
#include "time.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <math.h>
#include <ui/ui.h>

static const char *TAG = "PUBREMOTE-REMOTEINPUTS";

// TODO - from SETTINGS
#ifndef JOYSTICK_BUTTON_LEVEL
  #error "JOYSTICK_BUTTON_LEVEL must be defined"
#endif

RemoteData remote_data;
JoystickData joystick_data;
static button_handle_t gpio_btn_handle = NULL;

float convert_adc_to_axis(int adc_value, int min_val, int mid_val, int max_val, int deadband, float expo, bool invert) {
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

  return invert ? -axis : axis;
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
    uint64_t newTime = get_current_time_ms();
    bool trigger_sleep_disrupt = false;
    int16_t deadband = calibration_settings.deadband;
#if JOYSTICK_Y_ENABLED
    int16_t y_center = calibration_settings.y_center;
    int16_t y_max = calibration_settings.y_max;
    int16_t y_min = calibration_settings.y_min;
#endif
#if JOYSTICK_X_ENABLED
    int16_t x_center = calibration_settings.x_center;
    int16_t x_max = calibration_settings.x_max;
    int16_t x_min = calibration_settings.y_min;
#endif
    float expo = calibration_settings.expo;
    bool invert_y = calibration_settings.invert_y;
    esp_err_t read_err;
#if JOYSTICK_X_ENABLED
    int x_value;
    read_err = adc_oneshot_read(x_adc_handle, JOYSTICK_X_ADC, &x_value);

    if (read_err == ESP_OK) {
      joystick_data.x = x_value;
      float new_x = convert_adc_to_axis(x_value, x_min, x_center, x_max, deadband, expo, false);
      float curr_x = remote_data.js_x;

      if (new_x != curr_x) {
        remote_data.js_x = new_x;
        trigger_sleep_disrupt = true;
      }
    }
    else {
      ESP_LOGE(TAG, "Error reading X axis: %d", read_err);
    }

#endif

#if JOYSTICK_Y_ENABLED
    int y_value;
    read_err = adc_oneshot_read(y_adc_handle, JOYSTICK_Y_ADC, &y_value);

    if (read_err == ESP_OK) {

      joystick_data.y = y_value;
      float new_y = convert_adc_to_axis(y_value, y_min, y_center, y_max, deadband, expo, invert_y);
      float curr_y = remote_data.js_y;

      if (new_y != curr_y) {
        remote_data.js_y = new_y;
        trigger_sleep_disrupt = true;
      }
    }
    else {
      ESP_LOGE(TAG, "Error reading Y axis: %d", read_err);
    }

#endif

    if (trigger_sleep_disrupt) {
      reset_sleep_timer();
    }

    int64_t elapsed = get_current_time_ms() - newTime;
    if (elapsed > 0 && elapsed < INPUT_RATE_MS) {
      vTaskDelay(pdMS_TO_TICKS(INPUT_RATE_MS - elapsed));
    }
  }

  ESP_LOGI(TAG, "Thumbstick task ended");
  vTaskDelete(NULL);
}

void thumbstick_init() {
#if (JOYSTICK_Y_ENABLED || JOYSTICK_X_ENABLED)
  xTaskCreatePinnedToCore(thumbstick_task, "thumbstick_task", 4096, NULL, 20, NULL, 0);
#endif
}

static void button_single_click_cb(void *arg, void *usr_data) {
  ESP_LOGI(TAG, "BUTTON SINGLE CLICK");
  reset_sleep_timer();
}

static void button_down_cb(void *arg, void *usr_data) {
  ESP_LOGI(TAG, "BUTTON DOWN");
  remote_data.bt_c = 1;
}

static void button_up_cb(void *arg, void *usr_data) {
  ESP_LOGI(TAG, "BUTTON UP");
  remote_data.bt_c = 0;
}
static void button_double_click_cb(void *arg, void *usr_data) {
  ESP_LOGI(TAG, "BUTTON DOUBLE CLICK");

  // If stats screen is active, open main menu
  if (is_stats_screen_active()) {
    _ui_screen_change(&ui_MenuScreen, LV_SCR_LOAD_ANIM_OVER_TOP, 200, 0, &ui_MenuScreen_screen_init);
  }

  reset_sleep_timer();
}

void reset_button_state() {
  remote_data.bt_c = 0;
}

void buttons_deinit() {
  if (gpio_btn_handle) {
    iot_button_delete(gpio_btn_handle);
    gpio_btn_handle = NULL;
  }
}

void buttons_init() {
#if JOYSTICK_BUTTON_ENABLED
  // create gpio button
  button_config_t gpio_btn_cfg = {
      .type = BUTTON_TYPE_GPIO,
      .long_press_time = CONFIG_BUTTON_LONG_PRESS_TIME_MS,
      .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS,
      .gpio_button_config =
          {
              .gpio_num = PRIMARY_BUTTON,
              .active_level = JOYSTICK_BUTTON_LEVEL,
          },
  };

  if (gpio_btn_handle != NULL) {
    ESP_LOGW(TAG, "Initialize called with existing button config. Please deinit before calling init");
    return;
  }

  gpio_btn_handle = iot_button_create(&gpio_btn_cfg);
  if (gpio_btn_handle == NULL) {
    ESP_LOGE(TAG, "Button create failed");
  }

  iot_button_register_cb(gpio_btn_handle, BUTTON_PRESS_DOWN, button_down_cb, NULL);
  iot_button_register_cb(gpio_btn_handle, BUTTON_PRESS_UP, button_up_cb, NULL);
  iot_button_register_cb(gpio_btn_handle, BUTTON_SINGLE_CLICK, button_single_click_cb, NULL);
  iot_button_register_cb(gpio_btn_handle, BUTTON_DOUBLE_CLICK, button_double_click_cb, NULL);
#endif
}

void register_primary_button_cb(button_event_t event, button_cb_t cb) {
  iot_button_register_cb(gpio_btn_handle, event, cb, NULL);
}

void unregister_primary_button_cb(button_event_t event) {
  iot_button_unregister_cb(gpio_btn_handle, event);
}
