#ifndef __POWERMANAGEMENT_H
#define __POWERMANAGEMENT_H
#include "lvgl.h"

#ifndef BAT_ADC // Note: must be ADC1
  #error "BAT_ADC must be defined"
#endif

void reset_sleep_timer();
void init_power_management();
void enter_sleep();

typedef enum {
  CHARGING,
  ON,
  OFF,
} PowerState;

#endif