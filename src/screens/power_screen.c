#include "esp_log.h"
#include <remote/display.h>
#include <remote/settings.h>
#include <ui/ui.h>

static const char *TAG = "PUBREMOTE-POWER_SCREEN";

bool is_power_screen_active() {
  lv_obj_t *active_screen = lv_scr_act();
  return active_screen == ui_PowerScreen;
}

// Event handlers
void power_screen_loaded(lv_event_t *e) {
  ESP_LOGI(TAG, "Power screen loaded");
  lv_dropdown_set_selected(ui_AutoOffTime, settings.auto_off_time);
}

void power_screen_unloaded(lv_event_t *e) {
  ESP_LOGI(TAG, "Power screen unloaded");
}

void auto_off_select_change(lv_event_t *e) {
  int val = lv_dropdown_get_selected(ui_AutoOffTime);
  settings.auto_off_time = (uint8_t)(val & 0xFF);
}

void power_settings_save(lv_event_t *e) {
  save_auto_off_time();
}