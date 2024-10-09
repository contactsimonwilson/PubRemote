#include <ui/ui.h>
#ifndef __CALIBRATION_SCREEN_H
  #define __CALIBRATION_SCREEN_H

bool is_calibration_screen_active();

void calibration_screen_loaded(lv_event_t *e);
void calibration_screen_unloaded(lv_event_t *e);
void calibration_settings_save(lv_event_t *e);

#endif