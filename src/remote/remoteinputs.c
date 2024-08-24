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
#include "remote/router.h"
#include "rom/gpio.h"
#include "time.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <ui/ui.h>
static const char *TAG = "PUBMOTE-REMOTEINPUTS";

RemoteDataUnion remote_data;

#define X_STICK_DEADZONE 250
#define Y_STICK_DEADZONE 250

// For PS5 Hall
// float convert_adc_to_axis(int adc_value) {
//   // 0 - 4095 -> 1.0 to -1.0
//   return (float)(2047 - adc_value) / 2047.0f;
// }

// For Switch
float convert_adc_to_axis(int adc_value) {
  // 0 - 4095 -> 1.0 to -1.0
  return (float)(2047 - adc_value) / 2047.0f;
}

void thumbstick_task(void *pvParameters) {
  // Configure the ADC
  adc_oneshot_unit_handle_t adc2_handle;
  adc_oneshot_unit_init_cfg_t init_config = {
      .unit_id = ADC_UNIT_2,
      .ulp_mode = ADC_ULP_MODE_DISABLE,
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc2_handle));

  // Calibration
  //-------------ADC2 Calibration Init---------------//
  adc_cali_handle_t adc2_cali_handle = NULL;
  bool do_calibration2 = adc_calibration_init(ADC_UNIT_2, ADC_ATTEN_DB_12, &adc2_cali_handle);

  // Configure the ADC channel
  adc_oneshot_chan_cfg_t channel_config = {
      .bitwidth = ADC_BITWIDTH_12,
      .atten = ADC_ATTEN_DB_12,
  };
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc2_handle, X_STICK_CHANNEL, &channel_config));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc2_handle, Y_STICK_CHANNEL, &channel_config));

  while (1) {
    int x_value, y_value;
    ESP_ERROR_CHECK(adc_oneshot_read(adc2_handle, X_STICK_CHANNEL, &x_value));
    ESP_ERROR_CHECK(adc_oneshot_read(adc2_handle, Y_STICK_CHANNEL, &y_value));
    remote_data.data.js_x = 0;
    remote_data.data.js_y = 0;
    if (x_value > 2047 + X_STICK_DEADZONE || x_value < 2047 - X_STICK_DEADZONE) {
      start_or_reset_deep_sleep_timer(DEEP_SLEEP_DELAY_MS);
      remote_data.data.js_x = convert_adc_to_axis(x_value);
      printf("Thumbstick x value: %d\n", x_value);
      // printf("Thumbstick x-axis value: %f\n", remote_data.data.js_x);
    }
    if (y_value > 2047 + Y_STICK_DEADZONE || y_value < 2047 - Y_STICK_DEADZONE) {
      start_or_reset_deep_sleep_timer(DEEP_SLEEP_DELAY_MS);
      remote_data.data.js_y = convert_adc_to_axis(y_value);
      printf("Thumbstick y value: %d\n", y_value);
      // printf("Thumbstick y-axis value: %f\n", remote_data.data.js_y);
    }

    // printf("Thumbstick x-axis raw value: %d\n", x_value);
    // printf("Thumbstick y-axis raw value: %d\n", y_value);

    // printf("Thumbstick x-axis value: %f\n", remote_data.data.js_x);
    // printf("Thumbstick y-axis value: %f\n", remote_data.data.js_y);

    vTaskDelay(pdMS_TO_TICKS(100)); // Increase the delay to 100ms for better clarity in the output
  }

  ESP_LOGI(TAG, "Thumbstick task ended");
  vTaskDelete(NULL);
}

void init_thumbstick() {
  xTaskCreate(thumbstick_task, "thumbstick_task", 4096, NULL, 2, NULL);
}

static void button_single_click_cb(void *arg, void *usr_data) {
  ESP_LOGI(TAG, "BUTTON SINGLE CLICK");
  start_or_reset_deep_sleep_timer(DEEP_SLEEP_DELAY_MS);
  remote_data.data.bt_c = 1;

  // Start a timer to reset the button state after a certain duration
  esp_timer_handle_t reset_timer;
  esp_timer_create_args_t timer_args = {.callback = reset_button_state, .name = "reset_button_timer"};
  ESP_ERROR_CHECK(esp_timer_create(&timer_args, &reset_timer));
  ESP_ERROR_CHECK(esp_timer_start_once(reset_timer, 1000000)); // 1000ms delay
}

static void button_double_click_cb(void *arg, void *usr_data) {
  ESP_LOGI(TAG, "BUTTON DOUBLE CLICK");
  start_or_reset_deep_sleep_timer(DEEP_SLEEP_DELAY_MS);
  // router_show_screen("calibration");
}

static void button_long_press_cb(void *arg, void *usr_data) {
  ESP_LOGI(TAG, "BUTTON LONG PRESSS");
  esp_deep_sleep_start();
}

void reset_button_state() {
  remote_data.data.bt_c = 0;
}

void init_buttons() {
  // create gpio button
  button_config_t gpio_btn_cfg = {
      .type = BUTTON_TYPE_GPIO,
      .long_press_time = CONFIG_BUTTON_LONG_PRESS_TIME_MS,
      .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS,
      .gpio_button_config =
          {
              .gpio_num = JOYSTICK_BUTTON_PIN,
              .active_level = 1,
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