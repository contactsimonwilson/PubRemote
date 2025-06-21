#include "haptic.h"
#include "buzzer.h"
#include "config.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "settings.h"
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <nvs.h>

#ifdef HAPTIC_PWM
  #include "haptic/pwm/haptic_driver_pwm.h"
#elif HAPTIC_DRV2605
  #include "haptic/drv2605/haptic_driver_drv2605.hpp"
#endif

static const char *TAG = "PUBREMOTE-HAPTIC";

#if HAPTIC_ENABLED
// mutex for haptic
static SemaphoreHandle_t haptic_mutex;
#endif

// 0ms = play for pattern. Otherwise, play for duration_ms
static void play_vibration(HapticFeedbackPattern pattern, uint32_t duration_ms) {
#if HAPTIC_ENABLED
  // Take the mutex
  xSemaphoreTake(haptic_mutex, portMAX_DELAY);

  // Do stuff

  // Release the mutex
  xSemaphoreGive(haptic_mutex);
#endif
}

#if HAPTIC_ENABLED
// task to play melody
static void play_vibration_task(void *pvParameters) {
  play_vibration(HAPTIC_PATTERN_SHORT, 0); // Play a short vibration pattern

  vTaskDelete(NULL);
}
#endif

void vibrate() {
#if HAPTIC_ENABLED
  xTaskCreate(play_vibration_task, "play_vibration_task", 4096, NULL, 2, NULL);
#endif
}

void init_haptic() {
#if HAPTIC_ENABLED
  if (haptic_mutex == NULL) {
    haptic_mutex = xSemaphoreCreateMutex();
  }
  #if HAPTIC_DRV2605
  // Initialize the DRV2605 haptic driver
  ESP_LOGI(TAG, "Initializing DRV2605 haptic driver");
  drv2605_haptic_driver_init();
  #elif HAPTIC_PWM
  // Initialize the PWM haptic driver
  ESP_LOGI(TAG, "Initializing PWM haptic driver");
  pwm_haptic_driver_init();
  #endif
#endif
}