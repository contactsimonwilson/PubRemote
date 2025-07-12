#ifndef __REMOTEINPUTS_H
#define __REMOTEINPUTS_H
#include "adc.h"
#include "driver/gpio.h"
#include "iot_button.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <stdio.h>

void init_thumbstick();
void init_buttons();
void deinit_buttons();
void register_primary_button_cb(button_event_t event, button_cb_t cb);
void unregister_primary_button_cb(button_event_t event);

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