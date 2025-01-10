#ifndef __DISPLAY_H
#define __DISPLAY_H
#include "lvgl.h"
void display_task(void *pvParameters);

bool LVGL_lock(int timeout_ms);
void LVGL_unlock();
void init_display();
void deinit_display();
void set_bl_level(uint8_t level);
#endif