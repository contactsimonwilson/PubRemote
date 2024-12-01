#include "esp_log.h"
#include <remote/connection.h>
#include <remote/display.h>
#include <remote/receiver.h>
#include <ui/ui.h>

static const char *TAG = "PUBREMOTE-PAIRING_SCREEN";

bool is_pairing_screen_active() {
  lv_obj_t *active_screen = lv_scr_act();
  return active_screen == ui_PairingScreen;
}

// Event handlers
void pairing_screen_loaded(lv_event_t *e) {
  ESP_LOGI(TAG, "Pairing screen loaded");
  pairing_state = PAIRING_STATE_UNPAIRED;
  update_connection_state(CONNECTION_STATE_DISCONNECTED);
}

void pairing_screen_unloaded(lv_event_t *e) {
  ESP_LOGI(TAG, "Pairing screen unloaded");
  LVGL_lock(0);
  lv_label_set_text(ui_PairingCode, "0000");
  LVGL_unlock();
}