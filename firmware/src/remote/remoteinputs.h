#ifndef __REMOTEINPUTS_H
#define __REMOTEINPUTS_H
#include "adc.h"
#include "driver/gpio.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <stdio.h>

// Configuration
#ifndef JOYSTICK_BUTTON_PIN
  #define JOYSTICK_BUTTON_PIN -1
#endif

#ifndef JOYSTICK_X_ADC
  #define JOYSTICK_X_ADC -1
#endif

#ifndef JOYSTICK_Y_ADC
  #define JOYSTICK_Y_ADC -1
#endif

#if JOYSTICK_BUTTON_PIN < 0
  #define JOYSTICK_BUTTON_ENABLED 0
#else
  #define JOYSTICK_BUTTON_ENABLED 1
#endif

#if JOYSTICK_X_ADC < 0
  #define JOYSTICK_X_ENABLED 0
#else
  #define JOYSTICK_X_ENABLED 1
#endif

#if JOYSTICK_Y_ADC < 0
  #define JOYSTICK_Y_ENABLED 0
#else
  #define JOYSTICK_Y_ENABLED 1
#endif

#if (JOYSTICK_Y_ENABLED && !defined(JOYSTICK_Y_ADC_UNIT))
  #error "JOYSTICK_Y_ADC_UNIT must be defined"
#endif

#if (JOYSTICK_X_ENABLED && !defined(JOYSTICK_X_ADC_UNIT))
  #error "JOYSTICK_X_ADC_UNIT must be defined"
#endif

void init_thumbstick();
void init_buttons();
void reset_button_state();
void enable_power_button(bool enable);

float convert_adc_to_axis(int adc_value, int min_val, int mid_val, int max_val, int deadband, float expo, bool invert);

typedef struct {
  float js_y;
  float js_x;
  char bt_c;
  char bt_z;
  char is_rev;
} remote_data_t;

typedef union {
  remote_data_t data;
  uint8_t bytes[sizeof(remote_data_t)];
} RemoteDataUnion;

typedef struct {
  uint16_t x;
  uint16_t y;
} JoystickData;

extern RemoteDataUnion remote_data;
extern JoystickData joystick_data;

#endif