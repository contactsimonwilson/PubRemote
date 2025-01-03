#include "esp_log.h"
#include <remote/display.h>
#include <remote/powermanagement.h>
#include <remote/settings.h>
#include <ui/ui.h>

static const char *TAG = "PUBREMOTE-MENU_SCREEN";

bool is_menu_screen_active() {
  lv_obj_t *active_screen = lv_scr_act();
  return active_screen == ui_MenuScreen;
}

// Event handlers
void menu_screen_loaded(lv_event_t *e) {
  ESP_LOGI(TAG, "Menu screen loaded");
}

void menu_screen_unloaded(lv_event_t *e) {
  ESP_LOGI(TAG, "Menu screen unloaded");
}

void enter_deep_sleep(lv_event_t *e) {
  enter_sleep();
}