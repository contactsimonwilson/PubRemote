#include <ui/ui.h>
#ifndef __ABOUT_SCREEN_H
  #define __ABOUT_SCREEN_H

bool is_about_screen_active();

void about_screen_load_start(lv_event_t *e);
void about_screen_loaded(lv_event_t *e);
void about_screen_unloaded(lv_event_t *e);
void update_button_press(lv_event_t *e);

#endif