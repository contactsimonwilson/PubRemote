#include <ui/ui.h>
#ifndef __POWER_SCREEN_H
  #define __POWER_SCREEN_H

bool is_power_screen_active();

void power_screen_loaded(lv_event_t *e);
void power_screen_unloaded(lv_event_t *e);
void auto_off_select_change(lv_event_t *e);
void power_settings_save(lv_event_t *e);

#endif