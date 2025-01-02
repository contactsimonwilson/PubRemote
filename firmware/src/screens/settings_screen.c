#include "esp_log.h"
#include "utilities/number_utils.h"
#include "utilities/theme_utils.h"
#include <remote/display.h>
#include <remote/settings.h>
#include <ui/ui.h>

static const char *TAG = "PUBREMOTE-SETTINGS_SCREEN";

bool is_settings_screen_active() {
  lv_obj_t *active_screen = lv_scr_act();
  return active_screen == ui_SettingsScreen;
}

static void scroll_event_cb(lv_event_t *e) {
  lv_obj_t *cont = lv_event_get_target(e);
  uint8_t total_items = lv_obj_get_child_cnt(cont);

  lv_coord_t visible_height = lv_obj_get_width(cont);
  lv_coord_t scroll_x = lv_obj_get_scroll_x(cont);
  uint32_t page_index = (scroll_x + (visible_height / 2)) / visible_height;

  // Ensure we don't exceed total items
  uint8_t current_page = clampu8((uint8_t)page_index, 0, total_items - 1);

  if (LVGL_lock(-1)) {
    for (uint8_t i = 0; i < total_items; i++) {
      // get scroll indicator
      lv_obj_t *indicator = lv_obj_get_child(ui_SettingsHeader, i);
      lv_obj_set_style_bg_opa(indicator, i == current_page ? 255 : 100, LV_PART_MAIN);
    }
    LVGL_unlock();
  }
}

// Event handlers
void settings_screen_load_start(lv_event_t *e) {
  ESP_LOGI(TAG, "Settings screen load start");
  if (LVGL_lock(-1)) {
    // Set the scroll snap
    lv_obj_set_scroll_snap_x(ui_SettingsBody, LV_SCROLL_SNAP_CENTER);
    lv_obj_add_event_cb(ui_SettingsBody, scroll_event_cb, LV_EVENT_SCROLL, NULL);
    // lv_obj_scroll_to_x(ui_SettingsBody, 0, LV_ANIM_OFF);

    // Brightness
    lv_slider_set_value(ui_BrightnessSlider, device_settings.bl_level, LV_ANIM_OFF);

    // Auto off time
    lv_dropdown_set_selected(ui_AutoOffTime, device_settings.auto_off_time);

    // Temp units
    lv_dropdown_set_selected(ui_TempUnits, device_settings.temp_units);

    // Distance units
    lv_dropdown_set_selected(ui_DistanceUnits, device_settings.distance_units);

    // Theme color
    lv_color_t color = lv_color_hex(device_settings.theme_color);
    lv_colorwheel_set_rgb(ui_ThemeColor, color);
    LVGL_unlock();
  }
}

void settings_screen_loaded(lv_event_t *e) {
  ESP_LOGI(TAG, "Settings screen loaded");
}

void settings_screen_unloaded(lv_event_t *e) {
  ESP_LOGI(TAG, "Settings screen unloaded");
  lv_obj_remove_event_cb(ui_SettingsBody, scroll_event_cb);
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

void temp_units_select_change(lv_event_t *e) {
  int val = lv_dropdown_get_selected(ui_TempUnits);
  device_settings.temp_units = (uint8_t)(val & 0xFF);
}

void distance_units_select_change(lv_event_t *e) {
  int val = lv_dropdown_get_selected(ui_DistanceUnits);
  device_settings.distance_units = (uint8_t)(val & 0xFF);
}

void theme_color_picker_change(lv_event_t *e) {
  lv_color_t val = lv_colorwheel_get_rgb(ui_ThemeColor);
  device_settings.theme_color = lv_color_to32(val);
  reload_theme();
}

void settings_save(lv_event_t *e) {
  // Brightness
  save_device_settings();
}