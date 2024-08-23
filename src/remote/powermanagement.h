#ifndef __POWERMANAGEMENT_H
#define __POWERMANAGEMENT_H
#include <math.h>

#define DEEP_SLEEP_DELAY_MS 60000 // 1 minutes
static void deep_sleep_timer_callback(void *arg);
void start_or_reset_deep_sleep_timer(uint64_t duration_ms);
void init_power_management();
void check_button_press();

typedef enum {
  CHARGING,
  ON,
  OFF,
} PowerState;

#endif