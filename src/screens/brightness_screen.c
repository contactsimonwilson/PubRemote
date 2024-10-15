#include "esp_log.h"
#include <remote/display.h>
#include <remote/settings.h>
#include <ui/ui.h>

static const char *TAG = "PUBREMOTE-BRIGHTNESS_SCREEN";

bool is_brightness_screen_active() {
  lv_obj_t *active_screen = lv_scr_act();
  return active_screen == ui_BrightnessScreen;
}

// Event handlers
void brightness_screen_loaded(lv_event_t *e) {
  ESP_LOGI(TAG, "Brightness screen loaded");
  lv_slider_set_value(ui_BrightnessSlider, device_settings.bl_level, LV_ANIM_OFF);
}

void brightness_screen_unloaded(lv_event_t *e) {
  ESP_LOGI(TAG, "Brightness screen unloaded");
}

void brightness_slider_change(lv_event_t *e) {
  int val = lv_slider_get_value(ui_BrightnessSlider);
  device_settings.bl_level = (uint8_t)val;
  set_bl_level(device_settings.bl_level);
}

void brightness_save(lv_event_t *e) {
  save_bl_level();
}