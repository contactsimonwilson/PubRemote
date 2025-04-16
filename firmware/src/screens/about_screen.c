#include "esp_log.h"
#include "remote/display.h"
#include "remote/ota.h"
#include "utilities/screen_utils.h"
#include "utilities/string_utils.h"
#include <remote/remoteinputs.h>
#include <remote/stats.h>
#include <stdio.h>
#include <stdlib.h>
#include <ui/ui.h>

// Set in env
// Powershell: $env:PLATFORMIO_BUILD_FLAGS='-D RELEASE_VARIANT=\"release\"'
#ifndef RELEASE_VARIANT
  #define RELEASE_VARIANT "dev"
#endif

static const char *TAG = "PUBREMOTE-ABOUT_SCREEN";

bool is_about_screen_active() {
  lv_obj_t *active_screen = lv_scr_act();
  return active_screen == ui_AboutScreen;
}

// Set version label string
void update_version_info_label() {
  char *formattedString;
  asprintf(&formattedString, "Version: %d.%d.%d.%s\nType: %s\nHash: %s", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH,
           RELEASE_VARIANT, BUILD_TYPE, BUILD_ID);
  lv_label_set_text(ui_VersionInfoLabel, formattedString);
  free(formattedString);
}

void update_battery_percentage_label() {
  char *formattedString;
  asprintf(&formattedString, "Battery: %.2fV | %d%%", remoteStats.remoteBatteryVoltage,
           remoteStats.remoteBatteryPercentage);
  lv_label_set_text(ui_DebugInfoLabel, formattedString);
  free(formattedString);
}

void about_task(void *pvParameters) {
  while (is_about_screen_active()) {
    if (LVGL_lock(-1)) {
      update_battery_percentage_label();

      LVGL_unlock();
    }

    vTaskDelay(pdMS_TO_TICKS(LV_DISP_DEF_REFR_PERIOD));
  }

  ESP_LOGI(TAG, "About task ended");
  vTaskDelete(NULL);
}

// Event handlers
void about_screen_load_start(lv_event_t *e) {
  ESP_LOGI(TAG, "About screen load start");

  if (LVGL_lock(0)) {
    apply_ui_scale(NULL);

    update_version_info_label();
    update_battery_percentage_label();

    LVGL_unlock();
  }
}

void about_screen_loaded(lv_event_t *e) {
  ESP_LOGI(TAG, "About screen loaded");

  // Start task to update UI
  xTaskCreate(about_task, "about_task", 4096, NULL, 2, NULL);
}

void about_screen_unloaded(lv_event_t *e) {
  ESP_LOGI(TAG, "About screen unloaded");
}