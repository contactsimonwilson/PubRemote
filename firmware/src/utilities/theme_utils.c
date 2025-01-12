#include "lvgl.h"
#include "remote/buzzer.h"
#include <remote/settings.h>

/* Get and copy current theme */
static lv_theme_t *get_new_theme(lv_color_t color_primary, lv_color_t color_secondary) {
  /* Get the current theme */
  lv_disp_t *disp = lv_disp_get_default();
  // lv_theme_t *current_theme = lv_disp_get_theme(disp);

  /* Create a new theme based on the current one */
  lv_theme_t *new_theme = lv_theme_default_init(disp, color_primary, color_secondary, true, LV_FONT_DEFAULT);

  return new_theme;
}

static lv_theme_apply_cb_t orig_theme_apply_cb;

static void custom_theme_apply_cb(lv_theme_t *theme, lv_obj_t *obj) {
  // Apply original styles from theme
  if (orig_theme_apply_cb) {
    orig_theme_apply_cb(theme, obj);
  }

  // Apply custom styles
  if (lv_obj_check_type(obj, &lv_btn_class)) {
    // TODO - Use settings for white/black text
    lv_obj_set_style_text_color(obj, lv_color_hex(0x000000), LV_PART_MAIN);
  }
}

void reload_theme() {
  lv_color_t primary_color = lv_color_hex(device_settings.theme_color);
  lv_theme_t *new_theme = get_new_theme(primary_color, lv_palette_main(LV_PALETTE_RED));
  orig_theme_apply_cb = new_theme->apply_cb;
  lv_theme_set_apply_cb(new_theme, custom_theme_apply_cb);
  lv_disp_set_theme(lv_disp_get_default(), new_theme);
}