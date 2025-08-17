#ifndef __POWERMANAGEMENT_H
#define __POWERMANAGEMENT_H
#include "lvgl.h"

void acc_power_enable(bool enable);
void reset_sleep_timer();
void power_management_init();
void enter_sleep();
void bind_power_button();
void unbind_power_button();

typedef enum {
  CHARGING,
  ON,
  OFF,
} PowerState;

#endif