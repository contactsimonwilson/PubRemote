#include <ui/ui.h>
#ifndef __BRIGHTNESS_SCREEN_H
  #define __BRIGHTNESS_SCREEN_H

void brightness_screen_loaded(lv_event_t *e);
void brightness_slider_change(lv_event_t *e);
void brightness_save(lv_event_t *e);

#endif