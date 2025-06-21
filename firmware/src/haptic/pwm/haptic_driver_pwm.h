#ifndef __HAPTIC_DRIVER_PWM_H
#define __HAPTIC_DRIVER_PWM_H

#include "remote/haptic.h"

void pwm_haptic_play_vibration(HapticFeedbackPattern pattern);
void pwm_haptic_driver_init();

#endif