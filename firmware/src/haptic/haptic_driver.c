#include "haptic_driver.h"
#include "esp_log.h"
#ifdef HAPTIC_PWM
  #include "haptic/pwm/haptic_driver_pwm.h"
#elif HAPTIC_DRV2605
  #include "haptic/drv2605/haptic_driver_drv2605.hpp"
#endif

static const char *TAG = "PUBREMOTE-HAPTIC";

esp_err_t haptic_driver_init() {
#if HAPTIC_DRV2605
  // Initialize the DRV2605 haptic driver
  ESP_LOGI(TAG, "Initializing DRV2605 haptic driver");
  return drv2605_haptic_driver_init();
#elif HAPTIC_PWM
  // Initialize the PWM haptic driver
  ESP_LOGI(TAG, "Initializing PWM haptic driver");
  return pwm_haptic_driver_init();
#endif
}