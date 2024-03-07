#ifndef __REMOTEINPUTS_H
#define __REMOTEINPUTS_H
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <stdio.h>

void init_throttle();
void init_buttons();

#endif