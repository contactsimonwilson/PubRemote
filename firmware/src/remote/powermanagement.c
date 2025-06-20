#include "powermanagement.h"
#include "adc.h"
#include "buzzer.h"
#include "charge/charge_driver.h"
#include "config.h"
#include "display.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "gpio_detection.h"
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

static bool has_pull_resistor = true;

static bool get_use_light_sleep() {
  return FORCE_LIGHT_SLEEP || !has_pull_resistor || !gpio_supports_wakeup_from_deep_sleep(JOYSTICK_BUTTON_PIN);
}

static bool get_button_pressed() {
  return gpio_get_level(JOYSTICK_BUTTON_PIN) == JOYSTICK_BUTTON_LEVEL;
}

static bool check_button_press() {
  uint64_t pressStartTime = esp_timer_get_time();
  while (get_button_pressed()) { // Check if button is still pressed
    if ((esp_timer_get_time() - pressStartTime) >= (CONFIG_BUTTON_LONG_PRESS_TIME_MS * 1000)) {
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

  if (esp_sleep_is_valid_wakeup_gpio(JOYSTICK_BUTTON_PIN) && !get_use_light_sleep()) {
    res = esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK(JOYSTICK_BUTTON_PIN), JOYSTICK_BUTTON_LEVEL);
    if (res != ESP_OK) {
      ESP_LOGE(TAG, "Failed to enable wake-up on button press.");
    }
  }
  else {
    ESP_LOGI(TAG, "Button pin is not available for wake. Using light sleep instead.");
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

static void power_button_long_press_hold(void *arg, void *usr_data) {
  BoardState state = remoteStats.state;
  if (state == BOARD_STATE_RUNNING || state == BOARD_STATE_RUNNING_FLYWHEEL || state == BOARD_STATE_RUNNING_TILTBACK ||
      state == BOARD_STATE_RUNNING_UPSIDEDOWN || state == BOARD_STATE_RUNNING_WHEELSLIP) {
    ESP_LOGI(TAG, "Power button long press hold detected. Ignoring.");
    return;
  }

  // Immediately turn off screen but wait for release before sleep
  set_bl_level(0);

  while (get_button_pressed()) {
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  enter_sleep();
}

void bind_power_button() {
  register_primary_button_cb(BUTTON_LONG_PRESS_HOLD, power_button_long_press_hold);
}

void unbind_power_button() {
  unregister_primary_button_cb(BUTTON_LONG_PRESS_HOLD);
}

static void power_button_initial_release(void *arg, void *usr_data) {
  unregister_primary_button_cb(BUTTON_PRESS_UP);
  external_pull_t resistor = detect_gpio_external_pull(JOYSTICK_BUTTON_PIN);
  deinit_buttons();
  init_buttons(); // Reinit buttons after detect to ensure correct gpio config

  if (resistor == EXTERNAL_PULL_NONE || (resistor == EXTERNAL_PULL_DOWN && !JOYSTICK_BUTTON_LEVEL) ||
      (resistor == EXTERNAL_PULL_UP && JOYSTICK_BUTTON_LEVEL)) {
    has_pull_resistor = false;
  }
  else {
    has_pull_resistor = true;
  }

  bind_power_button();
}

void enter_sleep() {
  // Disable some things so they don't run during wake check
  update_connection_state(CONNECTION_STATE_DISCONNECTED);
  unbind_power_button();
  enable_wake();
  vTaskDelay(10); // Allow gpio level to settle before going into sleep
  // Turn off screen before sleep
  set_bl_level(0);

  while (1) {
    if (esp_sleep_is_valid_wakeup_gpio(JOYSTICK_BUTTON_PIN) && !get_use_light_sleep()) {
      ESP_LOGI(TAG, "Entering deep sleep mode");
      esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
      esp_deep_sleep_start(); // No code executes after esp_deep_sleep_start()
    }
    else {
      ESP_LOGI(TAG, "Entering light sleep mode");
      esp_light_sleep_start();

      // Light sleep resumes code execution after wake-up
      ESP_LOGI(TAG, "Woke up from light sleep mode");

      if (check_button_press()) {
        ESP_LOGI(TAG, "Button was pressed. Resuming from light sleep.");
        esp_restart(); // Restart the system so we start from the same point as deep sleep
      }

      // If button was not pressed, continue the loop
      ESP_LOGI(TAG, "Button was not pressed. Entering sleep mode again.");
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
    ESP_LOGD(TAG, "Deep sleep timer disabled.");
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
  ESP_ERROR_CHECK(charge_driver_init());
  vTaskDelay(pdMS_TO_TICKS(1000));
#define MIN_BATTERY_VOLTAGE 3000
#define MAX_BATTERY_VOLTAGE 4200

  while (1) {
    remoteStats.remoteBatteryVoltage = get_power_state().voltage;
    float battery_percentage = fmaxf(0, (float)(remoteStats.remoteBatteryVoltage - MIN_BATTERY_VOLTAGE)) /
                               (float)(MAX_BATTERY_VOLTAGE - MIN_BATTERY_VOLTAGE);
    remoteStats.remoteBatteryPercentage = clampu8((uint8_t)(battery_percentage * 100), 0, 100);
    ESP_LOGD(TAG, "Battery volts: %u %d", remoteStats.remoteBatteryVoltage, remoteStats.remoteBatteryPercentage);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  ESP_LOGI(TAG, "Power management task ended");
  // terminate self
  vTaskDelete(NULL);
}

void init_power_management() {
  init_sleep_timer();
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  ESP_LOGI(TAG, "Wake-up reason: %d", wakeup_reason);
  switch (wakeup_reason) {
  case ESP_SLEEP_WAKEUP_EXT0:
    ESP_LOGI(TAG, "Woken up by external signal on EXT0.");
    // Proceed to check if the button is still pressed
    if (!check_button_press()) {
      enter_sleep();
      return;
    }
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    ESP_LOGI(TAG, "Woken up by external signal on EXT1.");
    // Proceed to check if the button is still pressed
    if (!check_button_press()) {
      enter_sleep();
      return;
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
  if (get_button_pressed()) {
    // Enable the power button once released if it wasn't already
    register_primary_button_cb(BUTTON_PRESS_UP, power_button_initial_release);
  }
  else {
    power_button_initial_release(NULL, NULL);
  }

  xTaskCreate(power_management_task, "power_management_task", 4096, NULL, 2, NULL);
}