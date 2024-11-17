#include "esp_log.h"
#include <remote/display.h>
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
  // Brightness
  lv_slider_set_value(ui_BrightnessSlider, device_settings.bl_level, LV_ANIM_OFF);

  // Auto off time
  lv_dropdown_set_selected(ui_AutoOffTime, device_settings.auto_off_time);
}

void settings_screen_unloaded(lv_event_t *e) {
  ESP_LOGI(TAG, "Settings screen unloaded");
}

void brightness_slider_change(lv_event_t *e) {
  int val = lv_slider_get_value(ui_BrightnessSlider);
  device_settings.bl_level = (uint8_t)val;
  set_bl_level(device_settings.bl_level);
}

void auto_off_select_change(lv_event_t *e) {
  int val = lv_dropdown_get_selected(ui_AutoOffTime);
  device_settings.auto_off_time = (uint8_t)(val & 0xFF);
}

void settings_save(lv_event_t *e) {
  // Brightness
  save_bl_level();

  // Auto off time
  save_auto_off_time();
}