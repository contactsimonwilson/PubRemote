#ifndef __BUZZER_H
#define __BUZZER_H

#include "tones.h"
#include <stdio.h>

typedef enum {
  BUZZER_PATTERN_NONE,
  BUZZER_PATTERN_MELODY,
  BUZZER_PATTERN_SOLID,
} BuzzerPatttern;

void init_buzzer();
void set_buzzer_pattern(BuzzerPatttern pattern);
void set_buzzer_tone(BuzzerToneFrequency note, int duration);
void stop_buzzer();
void play_startup_tone();

#endif