#include <ui/ui.h>
#ifndef __SETTINGS_SCREEN_H
  #define __SETTINGS_SCREEN_H

bool is_settings_screen_active();

void settings_screen_loaded(lv_event_t *e);
void settings_screen_unloaded(lv_event_t *e);
void enter_deep_sleep(lv_event_t *e);

#endif