#include "powermanagement.h"
#include "adc.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "remoteinputs.h"
#include "settings.h"
#include <esp_adc/adc_oneshot.h>
#include <esp_err.h>
#include <esp_sleep.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <math.h>
#include <ui/ui.h>
static const char *TAG = "PUBREMOTE-POWERMANAGEMENT";

float convert_adc_to_battery_volts(int adc_value) {
  // 0 - 4095 -> 0 - 255
  float val = 3.3 / (1 << 12) * 3 * adc_value;
  return roundf(val * 10) / 10;
}

float BATTERY_VOLTAGE = 0;
#define BATTER_MONITOR_CHANNEL ADC_CHANNEL_0 // Assuming the Hall sensor is connected to GPIO0

#define REQUIRED_PRESS_TIME_MS 2000 // 2 seconds

void enter_sleep() {
  esp_deep_sleep_start();
}

void check_button_press() {
  uint64_t pressStartTime = esp_timer_get_time();
  while (gpio_get_level(JOYSTICK_BUTTON_PIN) == JOYSTICK_BUTTON_LEVEL) { // Check if button is still pressed
    if ((esp_timer_get_time() - pressStartTime) >= (REQUIRED_PRESS_TIME_MS * 1000)) {
      ESP_LOGI(TAG, "Button has been pressed for 2 seconds.");
      // Perform the desired action after confirmation of long press
      // maybe show start up screen or buzzer, etc.?
      break;
    }
    vTaskDelay(pdMS_TO_TICKS(10)); // Delay to allow for time checking without busy waiting
  }
  while (gpio_get_level(JOYSTICK_BUTTON_PIN) == JOYSTICK_BUTTON_LEVEL)
    ; // wait for button release
  if ((esp_timer_get_time() - pressStartTime) < (REQUIRED_PRESS_TIME_MS * 1000)) {
    enter_sleep(); // Go back to sleep if condition not met
  }
}

esp_timer_handle_t deep_sleep_timer;

static uint64_t get_sleep_timer_time_ms() {
  return get_auto_off_ms();
}

static void deep_sleep_timer_callback(void *arg) {
  // Enter deep sleep mode when the deep sleep timer expires
  ESP_LOGI(TAG, "Deep sleep timer expired. Entering deep sleep mode.");
  esp_deep_sleep_start();
}

void start_or_reset_deep_sleep_timer() {
  int duration_ms = get_sleep_timer_time_ms();

  if (duration_ms == 0) {
    ESP_LOGD(TAG, "Deep sleep timer disabled.");
    deep_sleep_timer = NULL;
    return;
  }

  if (deep_sleep_timer == NULL) {
    esp_timer_create_args_t deep_sleep_timer_args = {.callback = deep_sleep_timer_callback,
                                                     .arg = NULL,
                                                     .dispatch_method = ESP_TIMER_TASK,
                                                     .name = "DeepSleepTimer"};
    ESP_ERROR_CHECK(esp_timer_create(&deep_sleep_timer_args, &deep_sleep_timer));
  }
  else {
    ESP_ERROR_CHECK(esp_timer_stop(deep_sleep_timer));
    ESP_LOGD(TAG, "Deep sleep reset.");
  }
  ESP_ERROR_CHECK(esp_timer_start_once(deep_sleep_timer, duration_ms * 1000));
}

void power_management_task(void *pvParameters) {
  // Configure the ADC
  adc_oneshot_unit_handle_t adc1_handle;
  adc_oneshot_unit_init_cfg_t init_config = {
      .unit_id = ADC_UNIT_1,
      .ulp_mode = ADC_ULP_MODE_DISABLE,
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));

  // Calibration
  adc_cali_handle_t adc_cali_handle = NULL;
  bool do_calibration2 = adc_calibration_init(ADC_UNIT_1, ADC_ATTEN_DB_12, &adc_cali_handle);

  // Configure the ADC channel
  adc_oneshot_chan_cfg_t channel_config = {
      .bitwidth = ADC_BITWIDTH_12,
      .atten = ADC_ATTEN_DB_12,
  };
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, BATTER_MONITOR_CHANNEL, &channel_config));
  vTaskDelay(pdMS_TO_TICKS(1000));

  while (1) {
    int battery_value = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, BATTER_MONITOR_CHANNEL, &battery_value));
    BATTERY_VOLTAGE = convert_adc_to_battery_volts(battery_value);
    ESP_LOGD(TAG, "Battery volts: %.1f", BATTERY_VOLTAGE);
    // char str[20];
    // sprintf(str, "%.1f", BATTERY_VOLTAGE);
    //  lv_label_set_text(ui_PrimaryStat, str);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  ESP_LOGI(TAG, "Power management task ended");
  // terminate self
  vTaskDelete(NULL);
}

void init_power_management() {
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  esp_sleep_enable_ext0_wakeup(JOYSTICK_BUTTON_PIN, JOYSTICK_BUTTON_LEVEL);
  switch (wakeup_reason) {
  case ESP_SLEEP_WAKEUP_EXT0: { // Wake-up caused by external signal using RTC_IO
    ESP_LOGI(TAG, "Woken up by external signal on EXT0.");
    // Proceed to check if the button is still pressed
    check_button_press();
    break;
  }
  case ESP_SLEEP_WAKEUP_EXT1:
  case ESP_SLEEP_WAKEUP_TIMER:
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
  case ESP_SLEEP_WAKEUP_ULP:
  case ESP_SLEEP_WAKEUP_GPIO:
  case ESP_SLEEP_WAKEUP_UART:
    // Handle other wake-up sources if necessary
    break;
  default:
    ESP_LOGI(TAG, "Not a deep sleep wakeup or other wake-up sources.");
    break;
  }
  start_or_reset_deep_sleep_timer();
  xTaskCreate(power_management_task, "power_management_task", 4096, NULL, 2, NULL);
}