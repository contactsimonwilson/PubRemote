#ifndef __DISPLAY_H
#define __DISPLAY_H
#include "lvgl.h"
void display_task(void *pvParameters);

bool LVGL_lock(int timeout_ms);
void LVGL_unlock();
void init_display();
void set_bl_level(uint8_t level);
bool my_input_read(lv_indev_drv_t *drv, lv_indev_data_t *data);
#endif