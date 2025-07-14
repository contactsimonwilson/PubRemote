#ifndef __LED_H
#define __LED_H

#include <stdio.h>

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} RGB;

void init_led();
void set_led_brightness(uint8_t brightness);

#endif