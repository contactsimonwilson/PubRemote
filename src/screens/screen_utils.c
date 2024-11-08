#include "screen_utils.h"
#include "lvgl.h"
#include <ui/ui.h>
#define BASE_RES 240
#define SCALE_FACTOR ((float)LV_HOR_RES / BASE_RES)

static void scale_padding(lv_obj_t *obj) {
  if (obj == NULL)
    return;

  // Get current padding values
  lv_coord_t left = lv_obj_get_style_pad_left(obj, 0);
  lv_coord_t right = lv_obj_get_style_pad_right(obj, 0);
  lv_coord_t top = lv_obj_get_style_pad_top(obj, 0);
  lv_coord_t bottom = lv_obj_get_style_pad_bottom(obj, 0);

  // Scale and set new padding values using SCALE_FACTOR
  lv_obj_set_style_pad_left(obj, (lv_coord_t)(left * SCALE_FACTOR), LV_STATE_DEFAULT);
  lv_obj_set_style_pad_right(obj, (lv_coord_t)(right * SCALE_FACTOR), LV_STATE_DEFAULT);
  lv_obj_set_style_pad_top(obj, (lv_coord_t)(top * SCALE_FACTOR), LV_STATE_DEFAULT);
  lv_obj_set_style_pad_bottom(obj, (lv_coord_t)(bottom * SCALE_FACTOR), LV_STATE_DEFAULT);
}

static void scale_arc_width(lv_obj_t *obj) {
  if (obj == NULL || lv_obj_check_type(obj, &lv_arc_class) == false)
    return;

  // Get current arc width
  lv_coord_t width = lv_obj_get_style_arc_width(obj, 0);

  // Scale and set new arc width using SCALE_FACTOR
  lv_obj_set_style_arc_width(obj, (lv_coord_t)(width * SCALE_FACTOR), LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void scale_dimensions(lv_obj_t *obj) {
  if (obj == NULL) {
    return;
  }

  lv_coord_t width = lv_obj_get_style_width(obj, LV_PART_MAIN);
  lv_coord_t height = lv_obj_get_style_height(obj, LV_PART_MAIN);

  if (!LV_COORD_IS_PCT(width)) {
    lv_obj_set_width(obj, (lv_coord_t)(width * SCALE_FACTOR));
  }

  if (!LV_COORD_IS_PCT(height)) {
    lv_obj_set_height(obj, (lv_coord_t)(height * SCALE_FACTOR));
  }
}

static bool is_bold_font(lv_font_t *font) {
  if (font == &ui_font_Inter_Bold_14 || font == &ui_font_Inter_Bold_28 || font == &ui_font_Inter_Bold_48 ||
      font == &ui_font_Inter_Bold_96) {
    return true;
  }

  return false;
}

static uint8_t get_font_size(lv_font_t *font) {
  if (font == &ui_font_Inter_14 || font == &ui_font_Inter_Bold_14) {
    return 14;
  }
  else if (font == &ui_font_Inter_28 || font == &ui_font_Inter_Bold_28) {
    return 28;
  }
  else if (font == &ui_font_Inter_Bold_48) {
    return 48;
  }
  else if (font == &ui_font_Inter_Bold_96) {
    return 96;
  }
  return 14;
}

void scale_text(lv_obj_t *obj) {
  if (obj == NULL || !lv_obj_check_type(obj, &lv_label_class)) {
    return;
  }

  lv_font_t *font = lv_obj_get_style_text_font(obj, LV_PART_MAIN);
  int size = get_font_size(font) * SCALE_FACTOR;
  bool is_bold = is_bold_font(font);

  lv_font_t *new_font = font;
  if (size <= 20) {
    if (is_bold) {
      new_font = &ui_font_Inter_Bold_14;
    }
    else {
      new_font = &ui_font_Inter_28;
    }
  }
  else if (size <= 36) {
    if (is_bold) {
      new_font = &ui_font_Inter_Bold_28;
    }
    else {
      new_font = &ui_font_Inter_28;
    }
  }
  else if (size <= 64) {
    if (is_bold) {
      lv_obj_set_style_text_font(obj, &ui_font_Inter_Bold_48, LV_PART_MAIN);
    }
    else {
      lv_obj_set_style_text_font(obj, &ui_font_Inter_28, LV_PART_MAIN);
    }
  }
  else {
    if (is_bold) {
      lv_obj_set_style_text_font(obj, &ui_font_Inter_Bold_48, LV_PART_MAIN);
    }
    else {
      lv_obj_set_style_text_font(obj, &ui_font_Inter_28, LV_PART_MAIN);
    }
  }

  lv_obj_set_style_text_font(obj, new_font, LV_PART_MAIN);
}

void scale_element(lv_obj_t *element) {
  scale_padding(element);
  scale_dimensions(element);
  scale_arc_width(element);

  // Mark the style as modified
  lv_obj_mark_layout_as_dirty(element);
  lv_obj_refresh_style(element, LV_STYLE_PROP_ANY, LV_PART_MAIN);
  scale_text(element);
}

void apply_ui_scale() {
  // StatsScreen
  scale_element(ui_SpeedDial);
  scale_element(ui_UtilizationDial);
  scale_element(ui_LeftSensor);
  scale_element(ui_RightSensor);

  // SettingsScreen
  scale_element(ui_SettingsBackButton);
  // scale_element(ui_SettingsBackButtonLabel);
  scale_element(ui_SettingsBrightnessButton);
  scale_element(ui_SettingsPowerButton);
  scale_element(ui_SettingsCalibrateButton);
  scale_element(ui_SettingsPairButton);
  scale_element(ui_SettingsShutdownButton);
}
