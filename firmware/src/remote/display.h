#ifndef __DISPLAY_H
#define __DISPLAY_H
#include "lvgl.h"

#define BASE_RES 240

#define UI_SHAPE (LV_HOR_RES == LV_VER_RES)

#ifndef SCALE_UI
  #define SCALE_UI 1
#endif

#define SCALE_FACTOR (((float)LV_HOR_RES / BASE_RES) * SCALE_UI)

#ifndef SCALE_FONT
  #define SCALE_FONT 1
#endif

#ifndef SCALE_PADDING
  #define SCALE_PADDING 1
#endif

void display_task(void *pvParameters);

bool LVGL_lock(int timeout_ms);
void LVGL_unlock();
void init_display();
void deinit_display();
void set_bl_level(uint8_t level);
#endif