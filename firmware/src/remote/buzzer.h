#ifndef __BUZZER_H
#define __BUZZER_H

#include <stdio.h>

typedef enum {
  BUZZER_PATTERN_NONE,
  BUZZER_PATTERN_STARTUP_JINGLE,
  BUZZER_PATTERN_BEEP,
  BUZZER_PATTERN_SOLID,
} BuzzerPatttern;

void play_note(int frequency, int duration);
void play_melody();
void init_buzzer();
void play_startup_sound();

#endif