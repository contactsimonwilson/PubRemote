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
#include "remote/tones.h"
#include "remoteinputs.h"
#include "screens/charge_screen.h"
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
#include <remote/led.h>
#include <ui/ui.h>

static const char *TAG = "PUBREMOTE-POWERMANAGEMENT";

#define INT_SETTLE_TIME_MS 200

#ifdef PMU_INT
  #define PMU_INT_NOTE_DURATION 100
#endif

RTC_DATA_ATTR bool is_power_connected = false; // Store power state across deep sleep
static bool has_pull_resistor = true;

#ifdef PMU_INT
static QueueHandle_t pmu_evt_queue = NULL;

// Interrupt handler function (runs in IRAM)
static void IRAM_ATTR pmu_isr_handler(void *arg) {
  uint32_t gpio_num = (uint32_t)arg;
  xQueueSendFromISR(pmu_evt_queue, &gpio_num, NULL);
}
#endif

static void power_state_update() {
  RemotePowerState powerState = get_power_state();
  remoteStats.remoteBatteryVoltage = powerState.voltage;
  remoteStats.remoteBatteryPercentage = battery_mv_to_percent(remoteStats.remoteBatteryVoltage);
  remoteStats.chargeState = powerState.chargeState;
  remoteStats.chargeCurrent = powerState.current;
  ESP_LOGD(TAG, "Battery volts: %u %d", remoteStats.remoteBatteryVoltage, remoteStats.remoteBatteryPercentage);
  is_power_connected = powerState.isPowered;
}

static bool get_use_light_sleep() {
  return FORCE_LIGHT_SLEEP || !has_pull_resistor || !gpio_supports_wakeup_from_deep_sleep(PRIMARY_BUTTON);
}

static bool uses_deep_sleep() {
  return esp_sleep_is_valid_wakeup_gpio(PRIMARY_BUTTON) && !get_use_light_sleep();
}

static bool get_button_pressed() {
  return gpio_get_level(PRIMARY_BUTTON) == JOYSTICK_BUTTON_LEVEL;
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

static esp_err_t enable_wake() {
  esp_err_t res = ESP_OK;

  if (esp_sleep_is_valid_wakeup_gpio(PRIMARY_BUTTON) && !get_use_light_sleep()) {
    uint64_t io_mask = BIT64(PRIMARY_BUTTON);

    res = esp_sleep_enable_ext1_wakeup(io_mask,
                                       JOYSTICK_BUTTON_LEVEL ? ESP_EXT1_WAKEUP_ANY_HIGH : ESP_EXT1_WAKEUP_ANY_LOW);
    if (res != ESP_OK) {
      ESP_LOGE(TAG, "Failed to enable wake-up on button press.");
    }

    // Use PMU as secondary wake source if available
#ifdef PMU_INT
    res = esp_sleep_enable_ext0_wakeup(PMU_INT, 0);
    if (res != ESP_OK) {
      ESP_LOGE(TAG, "Failed to enable PMU interrupt wake-up.");
    }
#endif
  }
  else {
    ESP_LOGI(TAG, "Button pin is not available for wake. Using light sleep instead.");
    res = gpio_wakeup_enable(PRIMARY_BUTTON, JOYSTICK_BUTTON_LEVEL ? GPIO_INTR_HIGH_LEVEL : GPIO_INTR_LOW_LEVEL);
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
  display_set_bl_level(0);

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
  external_pull_t resistor = gpio_detect_external_pull(PRIMARY_BUTTON);
  debuttons_init();
  buttons_init(); // Reinit buttons after detect to ensure correct gpio config

  if (resistor == EXTERNAL_PULL_NONE || (resistor == EXTERNAL_PULL_DOWN && !JOYSTICK_BUTTON_LEVEL) ||
      (resistor == EXTERNAL_PULL_UP && JOYSTICK_BUTTON_LEVEL)) {
    has_pull_resistor = false;
  }
  else {
    has_pull_resistor = true;
  }

  bind_power_button();
}

#ifdef PMU_INT
static void await_pmu_int_reset() {
  // Wait for PMU_INT to go high
  int timeout = INT_SETTLE_TIME_MS;
  while (gpio_get_level(PMU_INT) == 0 && timeout > 0) {
    vTaskDelay(pdMS_TO_TICKS(10));
    timeout -= 10;
    ESP_LOGD(TAG, "Waiting for PMU_INT to go high...");
  }

  if (timeout <= 0) {
    ESP_LOGE(TAG, "PMU_INT did not go high within the expected time.");
  }
}
#endif

void enter_sleep() {
  // Disable some things so they don't run during wake check
  connection_update_state(CONNECTION_STATE_DISCONNECTED);
  unbind_power_button();

#if PMU_SY6970
  disable_watchdog();
#endif

  // Turn off screen before sleep
  display_off();
  led_set_brightness(0);
  enable_wake();
  vTaskDelay(50); // Allow gpio level to settle before going into sleep

#ifdef PMU_INT
  await_pmu_int_reset();
  power_state_update();
#endif

  while (1) {
    if (uses_deep_sleep()) {
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
#define POWER_MANAGEMENT_MAX_DELAY 1000 * 1000 // 1 second in microseconds
  static int64_t last_time = 0;

  while (1) {
#ifdef PMU_INT
    // Handle interrupts so we can react to power state changes almost immediately
    uint32_t io_num;
    while (xQueueReceive(pmu_evt_queue, &io_num, 0) == pdTRUE) {
      if (io_num == PMU_INT) {
        ESP_LOGD(TAG, "PMU interrupt received on GPIO %lu", io_num);
        bool last_power_connected = is_power_connected;
        power_state_update();
        last_time = esp_timer_get_time(); // Update last time in milliseconds

        if (is_power_connected != last_power_connected) {
          buzzer_set_tone(is_power_connected ? NOTE_SUCCESS : NOTE_ERROR, 300);
        }
      }
    }
#endif

    // Poll in 1s intervals
    int64_t current_time = esp_timer_get_time();
    if (current_time - last_time > POWER_MANAGEMENT_MAX_DELAY) {
      power_state_update();
      last_time = current_time;
    }

    // Todo - Check battery voltage and enter sleep if too low
    // if (remoteStats.remoteBatteryVoltage <= MIN_BATTERY_VOLTAGE && !is_power_connected) {
    //   ESP_LOGW(TAG, "Battery voltage too low: %d mV", remoteStats.remoteBatteryVoltage);
    //   play_note(NOTE_ERROR, 1000);
    //   // If battery is too low, enter sleep immediately
    //   enter_sleep();
    // }

    vTaskDelay(pdMS_TO_TICKS(100));
  }

  ESP_LOGI(TAG, "Power management task ended");
  // terminate self
  vTaskDelete(NULL);
}

#ifdef PMU_INT
static bool check_pmu_should_wake(bool last_powered) {
  await_pmu_int_reset();
  power_state_update();
  if (is_power_connected && !last_powered) {
    buzzer_set_tone(NOTE_SUCCESS, PMU_INT_NOTE_DURATION);
    // Power was connected after last sleep - continue to normal operation
  }
  else {
    if (!is_power_connected && last_powered) {
      buzzer_set_tone(NOTE_ERROR, PMU_INT_NOTE_DURATION);
    }
    enter_sleep();
    return false;
  }
  return true;
}
#endif

void power_management_init() {
  bool power_was_connected = is_power_connected;
  ESP_ERROR_CHECK(charge_driver_init());
  init_sleep_timer();
  vTaskDelay(pdMS_TO_TICKS(50)); // Allow time for peripherals to initialize
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
  power_state_update();

  ESP_LOGI(TAG, "Wake-up reason: %d", wakeup_reason);
  switch (wakeup_reason) {
  case ESP_SLEEP_WAKEUP_EXT0:
    ESP_LOGI(TAG, "Woken up by external signal on EXT0.");
    // Proceed to check if the button is still pressed
    if (!uses_deep_sleep()) {
      if (!check_button_press()) {
        enter_sleep();
        return;
      }
    }
    else {
#ifdef PMU_INT
      // Use PMU as secondary wake source if available
      ESP_LOGI(TAG, "Woken up by PMU interrupt.");
      if (!check_pmu_should_wake(power_was_connected)) {
        enter_sleep();
        return;
      }
#endif
    }

    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    ESP_LOGI(TAG, "Woken up by external signal on EXT1.");
    // Proceed to check if the button is still pressed
    if (wakeup_pin_mask & BIT64(PRIMARY_BUTTON)) {
      if (!check_button_press()) {
        enter_sleep();
        return;
      }
    }
#ifdef PMU_INT
    else if (wakeup_pin_mask & BIT64(PMU_INT)) {
      // Use PMU as secondary wake source if available
      ESP_LOGI(TAG, "Woken up by PMU interrupt.");
      if (!check_pmu_should_wake(power_was_connected)) {
        enter_sleep();
        return;
      }
    }
#endif
    else {
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

#ifdef PMU_INT
  gpio_config_t pmu_io_conf = {};
  pmu_io_conf.intr_type = GPIO_INTR_NEGEDGE;
  pmu_io_conf.mode = GPIO_MODE_INPUT;
  pmu_io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  pmu_io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  pmu_io_conf.pin_bit_mask = BIT64(PMU_INT);
  gpio_config(&pmu_io_conf);
  pmu_evt_queue = xQueueCreate(1, sizeof(uint32_t));

  gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
  gpio_isr_handler_add(PMU_INT, pmu_isr_handler, (void *)PMU_INT);
#endif

  xTaskCreate(power_management_task, "power_management_task", 4096, NULL, 2, NULL);
}