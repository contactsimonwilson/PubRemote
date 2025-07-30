#include "haptic.h"
#include "buzzer.h"
#include "config.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "haptic/haptic_driver.h"
#include "nvs_flash.h"
#include "remote/startup.h"
#include "settings.h"
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <nvs.h>

static const char *TAG = "PUBREMOTE-HAPTIC";

void haptic_vibrate(HapticFeedbackPattern pattern) {
#if HAPTIC_ENABLED
  ESP_LOGI(TAG, "Vibrating with pattern: %d", pattern);
  haptic_play_vibration(pattern);
#endif
}

void haptic_stop_vibration() {
#if HAPTIC_ENABLED
  ESP_LOGI(TAG, "Stopping vibration");
  haptic_haptic_stop_vibration();
#endif
}

static void haptic_play_startup_effect() {
#if HAPTIC_ENABLED
  haptic_vibrate(HAPTIC_SINGLE_CLICK);
  // Additional startup effects can be added here
#endif
}

void haptic_init() {
#if HAPTIC_ENABLED
  haptic_driver_init();
  register_startup_cb(haptic_play_startup_effect);
#endif
}