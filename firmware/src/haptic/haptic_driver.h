#ifndef __HAPTIC_DRIVER_H
#define __HAPTIC_DRIVER_H
#include "haptic_patterns.h"
#include <esp_err.h>

esp_err_t haptic_driver_init();
void haptic_play_vibration(HapticFeedbackPattern pattern);
void haptic_haptic_stop_vibration();

#endif