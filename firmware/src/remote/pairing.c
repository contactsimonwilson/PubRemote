#include "pairing.h"
#include "connection.h"
#include "esp_log.h"
#include "espnow.h"
#include "settings.h"
#include <esp_now.h>
#include <ui/ui.h>

static const char *TAG = "PUBREMOTE-PAIRING";

bool process_pairing_init(uint8_t *data, int len, esp_now_event_t evt) {
  if (len == 6) {
    uint8_t rec_mac[ESP_NOW_ETH_ALEN];
    memcpy(rec_mac, data, ESP_NOW_ETH_ALEN);
    if (!is_same_mac(evt.mac_addr, rec_mac)) {
      ESP_LOGE(TAG, "MAC Address mismatch on pairing request");
      return false;
    }
    memcpy(pairing_settings.remote_addr, rec_mac, ESP_NOW_ETH_ALEN);
    ESP_LOGI(TAG, "Got Pairing request from VESC Express");
    ESP_LOGI(TAG, "packet Length: %d", len);
    ESP_LOGI(TAG, "MAC Address: %02X:%02X:%02X:%02X:%02X:%02X", data[0], data[1], data[2], data[3], data[4], data[5]);
    // ESP_LOGI(TAG, "Incorrect MAC Address: %02X:%02X:%02X:%02X:%02X:%02X", mac_addr[0], mac_addr[1], mac_addr[2],
    //          mac_addr[3], mac_addr[4], mac_addr[5]);
    uint8_t TEST[1] = {420}; // TODO - FIX THIS
    // Do this internally as we don't want it to change connection state
    esp_now_peer_info_t peerInfo = {};
    peerInfo.channel = evt.chan; // Set the channel number (0-14)
    peerInfo.encrypt = false;
    memcpy(peerInfo.peer_addr, pairing_settings.remote_addr, sizeof(pairing_settings.remote_addr));
    pairing_settings.channel = evt.chan;
    // ESP_ERROR_CHECK(esp_now_add_peer(&peerInfo));
    uint8_t *mac_addr = pairing_settings.remote_addr;
    esp_err_t result = ESP_FAIL;

    if (channel_lock()) {
      if (esp_now_is_peer_exist(mac_addr)) {
        esp_err_t res = esp_now_del_peer(mac_addr);
        if (res != ESP_OK) {
          ESP_LOGE(TAG, "Failed to delete peer");
        }
      }

      esp_now_add_peer(&peerInfo);

      result = esp_now_send(mac_addr, (uint8_t *)&TEST, sizeof(TEST));
      channel_unlock();
    }

    if (result != ESP_OK) {
      // Handle error if needed
      ESP_LOGE(TAG, "Error sending pairing data: %d", result);
      return false;
    }
    else {
      ESP_LOGI(TAG, "Sent response back to VESC Express");
      pairing_state = PAIRING_STATE_PAIRING;
      return true;
    }
  }
  return false;
}

bool process_pairing_bond(uint8_t *data, int len) {
  ESP_LOGI(TAG, "Pairing bond");
  if (pairing_state == PAIRING_STATE_PAIRING && len == 4) {
    // grab secret code
    ESP_LOGI(TAG, "Grabbing secret code");
    ESP_LOGI(TAG, "packet Length: %d", len);
    pairing_settings.secret_code = (int32_t)(data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
    ESP_LOGI(TAG, "Secret Code: %li", pairing_settings.secret_code);
    char *formattedString;
    asprintf(&formattedString, "%ld", pairing_settings.secret_code);
    if (LVGL_lock(0)) {
      lv_label_set_text(ui_PairingCode, formattedString);
      LVGL_unlock();
    }
    free(formattedString);
    pairing_state = PAIRING_STATE_PENDING;
    return true;
  }
  return false;
}

bool process_pairing_complete(uint8_t *data, int len) {
  if (pairing_state == PAIRING_STATE_PENDING && len == 4) {
    // grab secret code
    ESP_LOGI(TAG, "Grabbing response");
    ESP_LOGI(TAG, "packet Length: %d", len);
    int response = (int32_t)(data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
    ESP_LOGI(TAG, "Response: %i", response);
    if (LVGL_lock(0)) {
      if (response == -1) {
        pairing_state = PAIRING_STATE_PAIRED;
        save_pairing_data();
        connect_to_default_peer();
        lv_disp_load_scr(ui_StatsScreen);
      }
      else {
        pairing_state = PAIRING_STATE_UNPAIRED;
      }
      LVGL_unlock();
    }
    return true;
  }
  return false;
}