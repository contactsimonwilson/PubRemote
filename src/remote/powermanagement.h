#ifndef __POWERMANAGEMENT_H
#define __POWERMANAGEMENT_H

void init_power_management();

typedef enum {
  CHARGING,
  ON,
  OFF,
} PowerState;

#endif