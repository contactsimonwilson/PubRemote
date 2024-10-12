#include "esp_log.h"
#include <remote/display.h>
#include <remote/powermanagement.h>
#include <remote/settings.h>
#include <ui/ui.h>

static const char *TAG = "PUBREMOTE-SETTINGS_SCREEN";

bool is_settings_screen_active() {
  lv_obj_t *active_screen = lv_scr_act();
  return active_screen == ui_SettingsScreen;
}

// Event handlers
void settings_screen_loaded(lv_event_t *e) {
  ESP_LOGI(TAG, "Settings screen loaded");
  lv_dropdown_set_selected(ui_AutoOffTime, device_settings.auto_off_time);
}

void settings_screen_unloaded(lv_event_t *e) {
  ESP_LOGI(TAG, "Settings screen unloaded");
}

void enter_deep_sleep(lv_event_t *e) {
  enter_sleep();
}