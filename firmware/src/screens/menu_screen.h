#include <ui/ui.h>
#ifndef __MENU_SCREEN_H
  #define __MENU_SCREEN_H

bool is_menu_screen_active();

void menu_screen_load_start(lv_event_t *e);
void menu_screen_loaded(lv_event_t *e);
void menu_screen_unload_start(lv_event_t *e);
void enter_deep_sleep(lv_event_t *e);
void menu_connect_press(lv_event_t *e);
void shutdown_button_long_press(lv_event_t *e);
void shutdown_button_down(lv_event_t *e);

#endif