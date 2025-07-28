#ifndef __HAPTIC_H
#define __HAPTIC_H
#include "haptic/haptic_driver.h"

void vibrate(HapticFeedbackPattern pattern);
void stop_vibration();
void init_haptic();

#endif