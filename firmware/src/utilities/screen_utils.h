#ifndef __SCREEN_UTILS_H
#define __SCREEN_UTILS_H
#include "lvgl.h"

void apply_ui_scale(lv_obj_t *element);
void reload_screens();
lv_group_t *create_navigation_group(lv_obj_t *container);

#endif