#include <ui/ui.h>
#ifndef __UPDATE_SCREEN_H
  #define __UPDATE_SCREEN_H

bool is_update_screen_active();

void update_screen_load_start(lv_event_t *e);
void update_screen_loaded(lv_event_t *e);
void update_screen_unload_start(lv_event_t *e);
void update_primary_button_press(lv_event_t *e);
void update_settings_secondary_button_press(lv_event_t *e);

#endif