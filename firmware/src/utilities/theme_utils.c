#include "lvgl.h"
#include "remote/buzzer.h"
#include <remote/settings.h>

static lv_theme_apply_cb_t theme_apply_cb;

static void custom_theme_apply_cb(lv_theme_t *theme, lv_obj_t *obj) {
  // Apply original styles from theme
  if (theme_apply_cb) {
    theme_apply_cb(theme, obj);
  }

  // Apply custom styles
  if (lv_obj_check_type(obj, &lv_btn_class)) {
    // Set dark text if needed
    if (device_settings.dark_text) {
      lv_obj_set_style_text_color(obj, lv_color_hex(0x000000), LV_PART_MAIN);
    }
    else {
      lv_obj_set_style_text_color(obj, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    }
  }
}

void reload_theme() {
  /* Set primary color from settings */
  lv_color_t primary_color = lv_color_hex(device_settings.theme_color);

  /* Create a new theme based on the current one */
  lv_theme_t *new_theme = lv_theme_default_init(lv_disp_get_default(), primary_color, lv_palette_main(LV_PALETTE_RED),
                                                true, LV_FONT_DEFAULT);

  /* Add theme apply callback function */
  theme_apply_cb = new_theme->apply_cb;
  lv_theme_set_apply_cb(new_theme, custom_theme_apply_cb);

  /* Set the new theme */
  lv_disp_set_theme(lv_disp_get_default(), new_theme);
}