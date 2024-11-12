#ifndef __BUZZER_H
#define __BUZZER_H

#include <stdio.h>

#ifndef BUZZER_PIN
  #define BUZZER_PIN -1
#endif

#if (BUZZER_PIN < 0)
  #define BUZZER_ENABLED 0
#else
  #define BUZZER_ENABLED 1
#endif

#ifndef BUZZER_INVERT
  #define BUZZER_INVERT 0
#endif

void play_melody();
void init_buzzer();

#endif