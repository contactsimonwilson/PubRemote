#ifndef __HAPTIC_H
#define __HAPTIC_H

#include <stdio.h>

#ifndef HAPTIC_EN_PIN
  #define HAPTIC_EN_PIN -1
#endif

#ifndef HAPTIC_INT_PIN
  #define HAPTIC_INT_PIN -1
#endif

#if (HAPTIC_EN_PIN < 0 && HAPTIC_INT_PIN < 0)
  #define HAPTIC_ENABLED 0
#else
  #define HAPTIC_ENABLED 1
#endif

void vibrate();
void init_haptic();

#endif