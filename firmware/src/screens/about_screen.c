#include "esp_log.h"
#include "remote/display.h"
#include <remote/stats.h>
#include <stdio.h>
#include <ui/ui.h>

static const char *TAG = "PUBREMOTE-ABOUT_SCREEN";

bool is_about_screen_active() {
  lv_obj_t *active_screen = lv_scr_act();
  return active_screen == ui_AboutScreen;
}

// Event handlers
void about_screen_load_start(lv_event_t *e) {
  ESP_LOGI(TAG, "About screen load start");
  LVGL_lock(0);

  // set the version number
  char *formattedString;
  asprintf(&formattedString, "Version: %d.%d.%d\nType: %s\nHash: %s", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH,
           BUILD_TYPE, BUILD_ID);

  lv_label_set_text(ui_VersionInfoLabel, formattedString);

  asprintf(&formattedString, "Battery: %.2fV", remoteStats.remoteBatteryVoltage);
  lv_label_set_text(ui_DebugInfoLabel, formattedString);
  LVGL_unlock();

  free(formattedString);
}

void about_screen_loaded(lv_event_t *e) {
  ESP_LOGI(TAG, "About screen loaded");
}

void about_screen_unloaded(lv_event_t *e) {
  ESP_LOGI(TAG, "About screen unloaded");
}