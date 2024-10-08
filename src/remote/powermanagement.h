#ifndef __POWERMANAGEMENT_H
#define __POWERMANAGEMENT_H

void start_or_reset_deep_sleep_timer();
void init_power_management();
void check_button_press();
void enter_sleep();

typedef enum {
  CHARGING,
  ON,
  OFF,
} PowerState;

#endif