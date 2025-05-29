#ifndef __REMOTEINPUTS_H
#define __REMOTEINPUTS_H
#include "adc.h"
#include "driver/gpio.h"
#include "iot_button.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <stdbool.h>
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
void deinit_buttons();
void reset_button_state();
void register_primary_button_cb(button_event_t event, button_cb_t cb);
void unregister_primary_button_cb(button_event_t event);

float convert_adc_to_axis(int adc_value, int min_val, int mid_val, int max_val, int deadband, float expo, bool invert);

typedef struct {
  float js_y;
  float js_x;
  bool bt_c;
  bool bt_z;
  bool is_rev;
} RemoteData;

typedef struct {
  uint16_t x;
  uint16_t y;
} JoystickData;

extern RemoteData remote_data;
extern JoystickData joystick_data;

#endif