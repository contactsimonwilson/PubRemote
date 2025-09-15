#include <ui/ui.h>
#ifndef __MENU_SCREEN_H
  #define __MENU_SCREEN_H

bool is_menu_screen_active();

void add_main_menu_navigation_buttons(lv_obj_t *body_item);
void menu_screen_load_start(lv_event_t *e);
void menu_screen_loaded(lv_event_t *e);
void menu_screen_unload_start(lv_event_t *e);
void enter_deep_sleep(lv_event_t *e);
void menu_back_press(lv_event_t *e);
void menu_connect_press(lv_event_t *e);
void menu_pocket_mode_press(lv_event_t *e);
void menu_settings_press(lv_event_t *e);
void menu_calibration_press(lv_event_t *e);
void menu_pair_press(lv_event_t *e);
void menu_about_press(lv_event_t *e);
void menu_shutdown_button_down(lv_event_t *e);
void menu_shutdown_button_press(lv_event_t *e);
void menu_shutdown_button_long_press(lv_event_t *e);

#endif