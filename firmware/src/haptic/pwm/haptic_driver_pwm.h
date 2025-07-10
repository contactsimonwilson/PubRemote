#ifndef __HAPTIC_DRIVER_PWM_H
#define __HAPTIC_DRIVER_PWM_H

#include "remote/haptic.h"
#include <esp_err.h>

void pwm_haptic_play_vibration(HapticFeedbackPattern pattern);
esp_err_t pwm_haptic_driver_init();

#endif