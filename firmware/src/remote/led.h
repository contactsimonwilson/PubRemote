#ifndef __LED_H
#define __LED_H

#include <stdio.h>

#ifndef LED_POWER_PIN
  #define LED_POWER_PIN -1
#endif

#ifndef LED_DATA_PIN
  #define LED_DATA_PIN -1
#endif

#if (LED_POWER_PIN < 0 || LED_DATA_PIN < 0)
  #define LED_ENABLED 0
#else
  #define LED_ENABLED 1
#endif

void init_led();

#endif