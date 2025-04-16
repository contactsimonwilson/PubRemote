#include "powermanagement.h"
#include "adc.h"
#include "buzzer.h"
#include "display.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "remoteinputs.h"
#include "settings.h"
#include "stats.h"
#include "utilities/number_utils.h"
#include <esp_adc/adc_oneshot.h>
#include <esp_err.h>
#include <esp_sleep.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <math.h>
#include <remote/connection.h>
#include <ui/ui.h>

static const char *TAG = "PUBREMOTE-POWERMANAGEMENT";

#ifndef BAT_ADC_F
  #define BAT_ADC_F 3
#endif

static esp_err_t init_battery_read() {
  return adc_oneshot_config_channel(adc1_handle, BAT_ADC, &adc_channel_config);
}

static float get_battery_voltage() {
#define NUM_SAMPLES 5
  uint8_t num_successful_samples = 0;
  int32_t accumulated = 0;

  for (int i = 0; i < NUM_SAMPLES; i++) {
    int sample = 0;
    int voltage_mv = 0;
    esp_err_t res = ESP_OK;
    res = adc_oneshot_read(adc1_handle, BAT_ADC, &sample);

    if (res != ESP_OK) {
      ESP_LOGE(TAG, "Failed to read battery voltage.");
      continue;
    }

    res = adc_cali_raw_to_voltage(adc1_cali_handle, sample, &voltage_mv);

    if (res != ESP_OK) {
      ESP_LOGE(TAG, "Failed to convert raw value to voltage.");
      continue;
    }

    accumulated += voltage_mv;
    num_successful_samples++;
    vTaskDelay(pdMS_TO_TICKS(1));
  }

  if (num_successful_samples == 0) {
    ESP_LOGE(TAG, "Failed to read battery voltage.");
    return 0.0f;
  }

  int battery_value = accumulated / num_successful_samples;
  return roundf(battery_value = 0.001f * battery_value * ((float)BAT_ADC_F) * 100.0f) / 100.0f;
}

#define REQUIRED_PRESS_TIME_MS 2000 // 2 seconds

#ifndef FORCE_LIGHT_SLEEP
  #define FORCE_LIGHT_SLEEP 0
#endif

static bool check_button_press() {
  uint64_t pressStartTime = esp_timer_get_time();
  while (gpio_get_level(JOYSTICK_BUTTON_PIN) == JOYSTICK_BUTTON_LEVEL) { // Check if button is still pressed
    if ((esp_timer_get_time() - pressStartTime) >= (REQUIRED_PRESS_TIME_MS * 1000)) {
      ESP_LOGI(TAG, "Button has been pressed for 2 seconds.");
      return true;
    }
    vTaskDelay(pdMS_TO_TICKS(10)); // Delay to allow for time checking without busy waiting
  }
  return false;
}

#define BUTTON_PIN_BITMASK(GPIO) (1ULL << GPIO) // 2 ^ GPIO_NUMBER in hex

static esp_err_t enable_wake() {
  esp_err_t res = ESP_OK;

  if (esp_sleep_is_valid_wakeup_gpio(JOYSTICK_BUTTON_PIN) && !FORCE_LIGHT_SLEEP) {
    res = esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK(JOYSTICK_BUTTON_PIN), JOYSTICK_BUTTON_LEVEL);
    if (res != ESP_OK) {
      ESP_LOGE(TAG, "Failed to enable wake-up on button press.");
    }
  }
  else {
    ESP_LOGI(TAG, "Button pin is not valid for wake-up. Using light sleep instead.");
    res = gpio_wakeup_enable(JOYSTICK_BUTTON_PIN, JOYSTICK_BUTTON_LEVEL ? GPIO_INTR_HIGH_LEVEL : GPIO_INTR_LOW_LEVEL);
    if (res != ESP_OK) {
      ESP_LOGE(TAG, "Failed to enable wake-up on button press.");
    }
    res = esp_sleep_enable_gpio_wakeup();
    if (res != ESP_OK) {
      ESP_LOGE(TAG, "Failed to enable wake-up on button press.");
    }
  }

  return res;
}

void enter_sleep() {
  vTaskDelay(50); // Delay for button debouncing
  if (esp_sleep_is_valid_wakeup_gpio(JOYSTICK_BUTTON_PIN) && !FORCE_LIGHT_SLEEP) {
    ESP_LOGI(TAG, "Entering deep sleep mode");
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_deep_sleep_start();
  }
  else {
    // Disable some things while in light sleep
    update_connection_state(CONNECTION_STATE_DISCONNECTED);
    enable_power_button(false);
    deinit_display();

    ESP_LOGI(TAG, "Entering light sleep mode");
    enable_wake();
    esp_light_sleep_start();

    // Light sleep resumes code execution after wake-up
    ESP_LOGI(TAG, "Woke up from light sleep mode");
    if (!check_button_press()) {
      ESP_LOGI(TAG, "Button was not pressed. Entering sleep mode.");
      enter_sleep();
    }
    else {
      ESP_LOGI(TAG, "Button was pressed. Resuming from light sleep.");
      // reinit everything
      enable_power_button(true);
      init_display();
      play_melody();
    }
  }
}

esp_timer_handle_t sleep_timer;
static SemaphoreHandle_t timer_mutex = NULL;

// Call this during initialization
static void init_sleep_timer() {
  timer_mutex = xSemaphoreCreateMutex();
  assert(timer_mutex != NULL);
}

static uint64_t get_sleep_timer_time_ms() {
  return get_auto_off_ms();
}

void sleep_timer_callback(void *arg) {
  // Take mutex if you're modifying shared resources
  if (xSemaphoreTake(timer_mutex, portMAX_DELAY) == pdTRUE) {
    // Enter deep sleep mode when the deep sleep timer expires
    ESP_LOGI(TAG, "Sleep timer expired. Entering sleep mode.");
    enter_sleep();

    xSemaphoreGive(timer_mutex);
  }
}

void reset_sleep_timer() {
  if (timer_mutex == NULL) {
    ESP_LOGE(TAG, "Timer mutex not initialized!");
    return;
  }

  if (xSemaphoreTake(timer_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
    ESP_LOGE(TAG, "Could not take timer mutex!");
    return;
  }

  int duration_ms = get_sleep_timer_time_ms();

  // Handle existing timer
  if (sleep_timer != NULL) {
    if (esp_timer_is_active(sleep_timer)) {
      ESP_ERROR_CHECK(esp_timer_stop(sleep_timer));
    }
    ESP_ERROR_CHECK(esp_timer_delete(sleep_timer));
    sleep_timer = NULL;
  }

  if (duration_ms == 0) {
    ESP_LOGI(TAG, "Deep sleep timer disabled.");
    xSemaphoreGive(timer_mutex);
    return;
  }

  // Create new timer
  esp_timer_create_args_t sleep_timer_args = {
      .callback = sleep_timer_callback, .arg = NULL, .dispatch_method = ESP_TIMER_TASK, .name = "SleepTimer"};
  ESP_ERROR_CHECK(esp_timer_create(&sleep_timer_args, &sleep_timer));
  ESP_ERROR_CHECK(esp_timer_start_once(sleep_timer, duration_ms * 1000));
  ESP_LOGD(TAG, "Sleep timer started for %d ms", duration_ms);

  xSemaphoreGive(timer_mutex);
}

void power_management_task(void *pvParameters) {
  ESP_ERROR_CHECK(init_battery_read());
  vTaskDelay(pdMS_TO_TICKS(1000));
#define MIN_BATTERY_VOLTAGE 3.0
#define MAX_BATTERY_VOLTAGE 4.2

  while (1) {
    remoteStats.remoteBatteryVoltage = get_battery_voltage();
    float battery_percentage = fmaxf(0, (remoteStats.remoteBatteryVoltage - MIN_BATTERY_VOLTAGE)) /
                               (MAX_BATTERY_VOLTAGE - MIN_BATTERY_VOLTAGE);
    remoteStats.remoteBatteryPercentage = clampu8((uint8_t)(battery_percentage * 100), 0, 100);
    ESP_LOGD(TAG, "Battery volts: %.1f %d", remoteStats.remoteBatteryVoltage, remoteStats.remoteBatteryPercentage);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  ESP_LOGI(TAG, "Power management task ended");
  // terminate self
  vTaskDelete(NULL);
}

void init_power_management() {
  enable_wake();
  init_sleep_timer();
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  ESP_LOGI(TAG, "Wake-up reason: %d", wakeup_reason);
  switch (wakeup_reason) {
  case ESP_SLEEP_WAKEUP_EXT0:
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    ESP_LOGI(TAG, "Woken up by external signal on EXT1.");
    // Proceed to check if the button is still pressed
    if (!check_button_press()) {
      enter_sleep();
    }
    break;
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
  reset_sleep_timer();
  xTaskCreate(power_management_task, "power_management_task", 4096, NULL, 2, NULL);
}