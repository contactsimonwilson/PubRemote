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

  // Mark the style as modified
  lv_obj_mark_layout_as_dirty(obj);
  lv_obj_refresh_style(obj, LV_STYLE_PROP_ANY, LV_PART_MAIN);
}

static void scale_arc_width(lv_obj_t *obj) {
  if (obj == NULL)
    return;

  // Get current arc width
  lv_coord_t width = lv_obj_get_style_arc_width(obj, 0);

  // Scale and set new arc width using SCALE_FACTOR
  lv_obj_set_style_arc_width(obj, (lv_coord_t)(width * SCALE_FACTOR), LV_PART_MAIN | LV_STATE_DEFAULT);

  // Mark the style as modified
  lv_obj_mark_layout_as_dirty(obj);
  lv_obj_refresh_style(obj, LV_STYLE_PROP_ANY, LV_PART_MAIN);
}

static void scale_container(lv_obj_t *container) {
}

static void scale_button(lv_obj_t *button) {
}

void scale_element(lv_obj_t *element) {
  scale_padding(element);
  scale_arc_width(element);
}

void apply_ui_scale() {
  scale_element(ui_SpeedDial);
  scale_element(ui_UtilizationDial);
  scale_element(ui_LeftSensor);
  scale_element(ui_RightSensor);
}
