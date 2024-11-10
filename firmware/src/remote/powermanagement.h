#ifndef __POWERMANAGEMENT_H
#define __POWERMANAGEMENT_H

#ifndef BAT_ADC // Note: must be ADC1
  #error "BAT_ADC must be defined"
#endif

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