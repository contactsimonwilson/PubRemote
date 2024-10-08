#include <remote/display.h>
#include <remote/settings.h>
#include <ui/ui.h>

static const char *TAG = "PUBREMOTE-BRIGHTNESS_SCREEN";

// Event handlers
void brightness_screen_loaded(lv_event_t *e) {
  lv_slider_set_value(ui_BrightnessSlider, settings.bl_level, LV_ANIM_OFF);
}

void brightness_slider_change(lv_event_t *e) {
  int val = lv_slider_get_value(ui_BrightnessSlider);
  settings.bl_level = (uint8_t)val;
  set_bl_level(settings.bl_level);
}

void brightness_save(lv_event_t *e) {
  save_bl_level();
}