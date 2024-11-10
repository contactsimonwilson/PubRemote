#include "esp_log.h"
#include <ui/ui.h>

static const char *TAG = "PUBREMOTE-ABOUT_SCREEN";

bool is_about_screen_active() {
  lv_obj_t *active_screen = lv_scr_act();
  return active_screen == ui_AboutScreen;
}

// Event handlers
void about_screen_loaded(lv_event_t *e) {
  ESP_LOGI(TAG, "About screen loaded");
}

void about_screen_unloaded(lv_event_t *e) {
  ESP_LOGI(TAG, "About screen unloaded");
}