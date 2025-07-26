#ifndef __LED_H
#define __LED_H

#include <stdio.h>

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} RGB;

typedef enum {
  LED_EFFECT_NONE,
  LED_EFFECT_PULSE,
  LED_EFFECT_SOLID,
} LedEffect;

void init_led();
void set_led_brightness(uint8_t brightness);

#endif