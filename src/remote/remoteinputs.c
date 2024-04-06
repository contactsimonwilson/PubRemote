#include "remoteinputs.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "iot_button.h"
#include "remote/router.h"
#include "rom/gpio.h"
#include "time.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <ui/ui.h>

static const char *TAG = "PUBMOTE-REMOTEINPUTS";
// Configuration
#define BUTTON_PIN GPIO_NUM_0
// #define BUTTON_PIN GPIO_NUM_15
#define Y_STICK_CHANNEL ADC_CHANNEL_5 // Assuming the Hall sensor is connected to GPIO0

uint8_t THROTTLE_VALUE = 128;
RemoteDataUnion remote_data;

uint8_t convert_adc_to_throttle(int adc_value) {
  // 0 - 4095 -> 0 - 255
  return (uint8_t)(adc_value / 16);
}

static bool example_adc_calibration_init(adc_unit_t unit, adc_atten_t atten, adc_cali_handle_t *out_handle) {
  adc_cali_handle_t handle = NULL;
  esp_err_t ret = ESP_FAIL;
  bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
  if (!calibrated) {
    ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = unit,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
    if (ret == ESP_OK) {
      calibrated = true;
    }
  }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
  if (!calibrated) {
    ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = unit,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
    if (ret == ESP_OK) {
      calibrated = true;
    }
  }
#endif

  *out_handle = handle;
  if (ret == ESP_OK) {
    ESP_LOGI(TAG, "Calibration Success");
  }
  else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
    ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
  }
  else {
    ESP_LOGE(TAG, "Invalid arg or no memory");
  }

  return calibrated;
}

void throttle_task(void *pvParameters) {
  // Configure the ADC
  adc_oneshot_unit_handle_t adc1_handle;
  adc_oneshot_unit_init_cfg_t init_config = {
      .unit_id = ADC_UNIT_2,
      .ulp_mode = ADC_ULP_MODE_DISABLE,
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));

  // Calibration
  //-------------ADC2 Calibration Init---------------//
  adc_cali_handle_t adc2_cali_handle = NULL;
  bool do_calibration2 = example_adc_calibration_init(ADC_UNIT_2, ADC_ATTEN_DB_12, &adc2_cali_handle);

  // Configure the ADC channel
  adc_oneshot_chan_cfg_t channel_config = {
      .bitwidth = ADC_BITWIDTH_12,
      .atten = ADC_ATTEN_DB_12,
  };
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, Y_STICK_CHANNEL, &channel_config));

  // TODO - IMPLEMENT ADC CONTINUOUS READING
  remote_data.data.js_y = 0.69f;
  remote_data.data.js_x = 0.69f;

  while (1) {
    int hall_value = 1337;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, Y_STICK_CHANNEL, &hall_value));
    THROTTLE_VALUE = convert_adc_to_throttle(hall_value);
    // printf("Throttle value: %d\n", THROTTLE_VALUE);
    //  Delay for 1 second
    vTaskDelay(pdMS_TO_TICKS(10));
  }

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
  router_show_screen("calibration");
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