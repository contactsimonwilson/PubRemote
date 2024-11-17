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

void reload_theme() {
  lv_color_t primary_color = lv_color_hex(device_settings.theme_color);
  // Use some settings from ui.c
  lv_theme_t *new_theme = get_new_theme(primary_color, lv_palette_main(LV_PALETTE_RED));
  lv_disp_set_theme(lv_disp_get_default(), new_theme);
}