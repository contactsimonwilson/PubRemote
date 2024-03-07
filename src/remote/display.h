#ifndef __DISPLAY_H
#define __DISPLAY_H
void display_task(void *pvParameters);

void init_display();
void set_bl_level(u_int8_t level);
#endif