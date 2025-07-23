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

void init_thumbstick();
void init_buttons();
void deinit_buttons();
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