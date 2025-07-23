#ifndef __SETTINGS_SCREEN_H
#define __SETTINGS_SCREEN_H

#include <ui/ui.h>
bool is_settings_screen_active();

void settings_screen_load_start(lv_event_t *e);
void settings_screen_loaded(lv_event_t *e);
void settings_screen_unloaded(lv_event_t *e);
void brightness_slider_change(lv_event_t *e);
void auto_off_select_change(lv_event_t *e);
void temp_units_select_change(lv_event_t *e);
void distance_units_select_change(lv_event_t *e);
void startup_sound_select_change(lv_event_t *e);
void theme_color_picker_change(lv_event_t *e);
void dark_text_switch_change(lv_event_t *e);
void settings_save(lv_event_t *e);

#endif