#ifndef __POWERMANAGEMENT_H
#define __POWERMANAGEMENT_H
#include "lvgl.h"

void reset_sleep_timer();
void init_power_management();
void enter_sleep();
void bind_power_button();
void unbind_power_button();

typedef enum {
  CHARGING,
  ON,
  OFF,
} PowerState;

#endif