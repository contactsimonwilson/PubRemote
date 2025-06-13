#include "buzzer.h"
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

static const char *TAG = "PUBREMOTE-HAPTIC";

#if HAPTIC_ENABLED
// mutex for haptic
static SemaphoreHandle_t haptic_mutex;
#endif

static void play_vibration(uint8_t pattern) {
#if HAPTIC_ENABLED
  // Take the mutex
  if (haptic_mutex == NULL) {
    haptic_mutex = xSemaphoreCreateMutex();
  }
  xSemaphoreTake(haptic_mutex, portMAX_DELAY);

  // Do stuff

  // Release the mutex
  xSemaphoreGive(buzzer_mutex);
#endif
}

#if HAPTIC_ENABLED
// task to play melody
static void play_vibration_task(void *pvParameters) {
  play_vibration(0); // Example pattern, replace with actual pattern logic

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
  //
#endif
}