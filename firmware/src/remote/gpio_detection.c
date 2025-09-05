#include "gpio_detection.h"

static const char *TAG = "PUBREMOTE-GPIO_DETECTION";

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