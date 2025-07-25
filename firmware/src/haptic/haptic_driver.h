#ifndef __HAPTIC_DRIVER_H
#define __HAPTIC_DRIVER_H
#include <esp_err.h>

typedef enum {
  HAPTIC_SINGLE_CLICK, ///< Single strong click
  HAPTIC_DOUBLE_CLICK, ///< Two quick clicks
  HAPTIC_TRIPLE_CLICK, ///< Three quick clicks
  HAPTIC_SOFT_BUMP,    ///< Gentle bump sensation
  HAPTIC_SOFT_BUZZ,    ///< Soft buzzing vibration
  HAPTIC_STRONG_BUZZ,  ///< Strong buzzing vibration
  HAPTIC_ALERT_750MS,  ///< Alert pattern 750ms
  HAPTIC_ALERT_1000MS  ///< Alert pattern 1000ms
} HapticFeedbackPattern;

void haptic_play_vibration(HapticFeedbackPattern pattern);

esp_err_t haptic_driver_init();

#endif