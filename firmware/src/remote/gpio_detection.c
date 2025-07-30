#include "gpio_detection.h"

static const char *TAG = "PUBREMOTE-GPIO_DETECTION";

#define GPIO_SETTLE_TIME 10

// Detect if a GPIO pin has an external pull-up or pull-down resistor
external_pull_t gpio_detect_external_pull(int gpio_num) {
  external_pull_t result = EXTERNAL_PULL_NONE;

  // Step 1: Configure GPIO as input with no pull-up/down
  gpio_config_t io_conf = {.pin_bit_mask = (1ULL << gpio_num),
                           .mode = GPIO_MODE_INPUT,
                           .pull_up_en = 0,
                           .pull_down_en = 0,
                           .intr_type = GPIO_INTR_DISABLE};
  gpio_config(&io_conf);

  // Short delay to allow pin to settle
  vTaskDelay(pdMS_TO_TICKS(GPIO_SETTLE_TIME));

  // Step 2: Read initial state
  int initial_level = gpio_get_level(gpio_num);
  ESP_LOGI(TAG, "Initial level of GPIO%d: %d", gpio_num, initial_level);

  if (initial_level == 1) {
    // Pin is high, test for external pull-up by enabling internal pull-down
    io_conf.pull_down_en = 1;
    gpio_config(&io_conf);

    // Short delay to allow pin to settle
    vTaskDelay(pdMS_TO_TICKS(GPIO_SETTLE_TIME));

    // Read pin with internal pull-down enabled
    int new_level = gpio_get_level(gpio_num);
    ESP_LOGI(TAG, "Level with internal pull-down: %d", new_level);

    if (new_level == 1) {
      // Pin stayed high despite internal pull-down, must have external pull-up
      result = EXTERNAL_PULL_UP;
    }
  }
  else {
    // Pin is low, test for external pull-down by enabling internal pull-up
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    // Short delay to allow pin to settle
    vTaskDelay(pdMS_TO_TICKS(GPIO_SETTLE_TIME));

    // Read pin with internal pull-up enabled
    int new_level = gpio_get_level(gpio_num);
    ESP_LOGI(TAG, "Level with internal pull-up: %d", new_level);

    if (new_level == 0) {
      // Pin stayed low despite internal pull-up, must have external pull-down
      result = EXTERNAL_PULL_DOWN;
    }
  }

  const char *result_str = "";
  switch (result) {
  case EXTERNAL_PULL_NONE:
    result_str = "No external pull detected";
    break;
  case EXTERNAL_PULL_UP:
    result_str = "External pull-up detected";
    break;
  case EXTERNAL_PULL_DOWN:
    result_str = "External pull-down detected";
    break;
  default:
    result_str = "Error detecting external pull";
    result = EXTERNAL_PULL_ERROR;
    break;
  }

  ESP_LOGI(TAG, "Result for GPIO%d: %s", gpio_num, result_str);
  return result;
}

// Check if a GPIO pin supports wakeup from deep sleep on ESP32-S3 or ESP32-C3
bool gpio_supports_wakeup_from_deep_sleep(gpio_num_t gpio_num) {
  // Common invalid pins for deep sleep wakeup across most ESP32 variants
  if (gpio_num == GPIO_NUM_NC) {
    return false;
  }

#if defined(CONFIG_IDF_TARGET_ESP32S3)
  // ESP32-S3 specific constraints

  // USB pins cannot be used for wakeup
  if (gpio_num == 19 || gpio_num == 20) {
    return false;
  }

  // Flash/PSRAM connected pins cannot be used
  if (gpio_num >= 26 && gpio_num <= 32) {
    return false;
  }

  // Strapping pins have limitations
  if (gpio_num == 0 || gpio_num == 3 || gpio_num == 45 || gpio_num == 46) {
    return false; // Can be used with caution, but better to avoid
  }

#elif defined(CONFIG_IDF_TARGET_ESP32C3)
  // ESP32-C3 specific constraints

  // Flash connected pins cannot be used
  if (gpio_num >= 11 && gpio_num <= 17) {
    return false;
  }

  // Strapping pins have limitations
  if (gpio_num == 2 || gpio_num == 8 || gpio_num == 9) {
    return false; // Can be used with caution, but better to avoid
  }
#endif

  // If we've passed all the checks, the pin supports wakeup
  return true;
}