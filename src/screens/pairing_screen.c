#include "esp_log.h"
#include <remote/display.h>
#include <remote/receiver.h>
#include <remote/settings.h>
#include <ui/ui.h>

static const char *TAG = "PUBREMOTE-PAIRING_SCREEN";

bool is_pairing_screen_active() {
  lv_obj_t *active_screen = lv_scr_act();
  return active_screen == ui_PairingScreen;
}

// Event handlers
void pairing_screen_loaded(lv_event_t *e) {
  ESP_LOGI(TAG, "Pairing screen loaded");
  pairing_settings.state = PAIRING_STATE_UNPAIRED;
}

void pairing_screen_unloaded(lv_event_t *e) {
  ESP_LOGI(TAG, "Pairing screen unloaded");
  save_pairing_data();
}