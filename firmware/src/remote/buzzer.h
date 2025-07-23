#ifndef __BUZZER_H
#define __BUZZER_H

#include <stdio.h>

void play_note(int frequency, int duration);
void play_melody();
void init_buzzer();
void play_startup_sound();

#endif