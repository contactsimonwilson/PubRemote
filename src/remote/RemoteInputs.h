#ifndef __REMOTEINPUTS_H
#define __REMOTEINPUTS_H
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <stdio.h>

typedef enum {
  BUTTON_NONE,
  BUTTON_CLICK,
  BUTTON_DOUBLE_CLICK,
  BUTTON_LONG_PRESS,
} ButtonState;

extern u_int8_t THROTTLE_VALUE;
extern ButtonState BUTTON_STATE;
extern TaskHandle_t buttonTaskHandle;

void throttle_task(void *pvParameters);
void button_task(void *pvParameters);
void register_button_isr();

#endif