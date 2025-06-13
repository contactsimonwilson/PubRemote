#ifndef __HAPTIC_H
#define __HAPTIC_H

#ifndef HAPTIC_EN_PIN
  #define HAPTIC_EN_PIN -1
#endif

#ifndef HAPTIC_INT_PIN
  #define HAPTIC_INT_PIN -1
#endif

#ifndef HAPTIC_PWM_PIN
  #define HAPTIC_PWM_PIN -1
#endif

#ifndef HAPTIC_DRV2605
  #define HAPTIC_DRV2605 0
#endif

#ifndef HAPTIC_PWM
  #define HAPTIC_PWM -1
#endif

#if (HAPTIC_EN_PIN > -1 || HAPTIC_PWM_PIN > -1)
  #define HAPTIC_ENABLED 1
#else
  #define HAPTIC_ENABLED 0
#endif

void vibrate();
void init_haptic();

#endif