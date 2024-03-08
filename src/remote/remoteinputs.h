#ifndef __REMOTEINPUTS_H
#define __REMOTEINPUTS_H
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <stdio.h>

void init_throttle();
void init_buttons();

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

typedef enum {
  BUTTON_NONE,
  BUTTON_CLICK,
  BUTTON_DOUBLE_CLICK,
  BUTTON_LONG_PRESS,
} ButtonState;

extern u_int8_t THROTTLE_VALUE;
extern ButtonState BUTTON_STATE;
extern TaskHandle_t buttonTaskHandle;
extern RemoteDataUnion remote_data;

void throttle_task(void *pvParameters);
void button_task(void *pvParameters);
void register_button_isr();

#endif