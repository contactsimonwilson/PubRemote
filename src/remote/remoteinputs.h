#ifndef __REMOTEINPUTS_H
#define __REMOTEINPUTS_H
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <stdio.h>

// Configuration
#define JOYSTICK_BUTTON_PIN GPIO_NUM_15
#define X_STICK_CHANNEL ADC_CHANNEL_6 // GPIO 17
#define Y_STICK_CHANNEL ADC_CHANNEL_5 // GPIO 16

void init_thumbstick();
void init_buttons();
void reset_button_state();

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

extern RemoteDataUnion remote_data;

#endif