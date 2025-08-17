#ifndef __SCREEN_UTILS_H
#define __SCREEN_UTILS_H
#include "lvgl.h"

void apply_ui_scale(lv_obj_t *element);
void reload_screens();
lv_group_t *create_navigation_group(lv_obj_t *container);
void add_page_scroll_indicators(lv_obj_t *header_item, lv_obj_t *body_item);
void paged_scroll_event_cb(lv_event_t *e);
void resize_footer_buttons(lv_obj_t *footer);

#endif