#ifndef __HAPTIC_H
#define __HAPTIC_H

typedef enum {
  HAPTIC_PATTERN_SHORT,
  HAPTIC_PATTERN_LONG,
  HAPTIC_PATTERN_ERROR,
  HAPTIC_PATTERN_SUCCESS
} HapticFeedbackPattern;

void vibrate();
void init_haptic();

#endif