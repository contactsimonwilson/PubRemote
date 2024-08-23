#ifndef __DISPLAY_H
#define __DISPLAY_H
#include "lvgl.h"
void display_task(void *pvParameters);

void init_display();
void set_bl_level(u_int8_t level);
bool my_input_read(lv_indev_drv_t *drv, lv_indev_data_t *data);
#endif