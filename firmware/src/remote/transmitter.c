#include "transmitter.h"
#include "commands.h"
#include "connection.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "peers.h"
#include "receiver.h"
#include "remoteinputs.h"
#include "screens/stats_screen.h"
#include "time.h"
#include <remote/settings.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "PUBREMOTE-TRANSMITTER";
#define COMMAND_TIMEOUT 1000

static void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // This callback runs in WiFi task context!
  ESP_LOGD(TAG, "SENT");
}

static uint8_t get_peer_channel(const uint8_t *peer_mac) {
  esp_now_peer_info_t peer_info = {0};

  esp_err_t result = esp_now_get_peer(peer_mac, &peer_info);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "Failed to get peer info: %s", esp_err_to_name(result));
    return 0; // Return 0 to indicate error
  }

  return peer_info.channel;
}

// Function to send ESP-NOW data
static void transmitter_task(void *pvParameters) {
  ESP_ERROR_CHECK(esp_now_register_send_cb(on_data_sent));
  ESP_LOGI(TAG, "Registered RX callback");

  ESP_LOGI(TAG, "TX task started");
  uint8_t ind = 0;
  uint8_t data[100];

  while (1) {
    int64_t newTime = get_current_time_ms();

    // Async/await API disabled for now
    // // Check if the last command was sent less than 1000ms ago
    // if (newTime - LAST_COMMAND_TIME < COMMAND_TIMEOUT) {
    //   vTaskDelay(pdMS_TO_TICKS(TX_RATE_MS));
    //   continue;
    // }

    if (is_stats_screen_active() && !is_pocket_mode_enabled() &&
        (connection_state == CONNECTION_STATE_CONNECTED || connection_state == CONNECTION_STATE_RECONNECTING ||
         connection_state == CONNECTION_STATE_CONNECTING)) {
      // Create a new buffer to hold both secret_Code and remote_data.bytes
      // printf("Thumbstick x-axis value: %f\n", remote_data.js_x);
      // printf("Thumbstick y-axis value: %f\n", remote_data.js_y);
      // Copy secret_Code to the beginning of the buffer
      data[0] = REM_REC_SET_REMOTE_STATE;
      ind++;

      memcpy(data + ind, &pairing_settings.secret_code, sizeof(int32_t));
      ind += sizeof(int32_t);

      // Copy remote_data.bytes after secret_Code
      memcpy(data + ind, &remote_data, sizeof(remote_data));
      ind += sizeof(remote_data);

      uint8_t *mac_addr = pairing_settings.remote_addr;
      if (channel_lock()) {
        esp_err_t result = esp_now_send(mac_addr, data, ind);
        if (result != ESP_OK) {
          // Handle error if needed
          uint8_t chann = pairing_settings.channel;
          uint8_t wifi_chann;
          wifi_second_chan_t secondary_channel;
          uint8_t peer_chann = get_peer_channel(mac_addr);
          esp_wifi_get_channel(&wifi_chann, &secondary_channel);
          ESP_LOGE(TAG, "Error sending remote data: %d  - Channel: %d, WiFi Channel: %d, Peer Channel: %d", result,
                   chann, wifi_chann, peer_chann);
        }
        channel_unlock();
      }

      LAST_COMMAND_TIME = newTime;
      ESP_LOGD(TAG, "Sent command");
    }
    // Reset the index for the next data packet and clear the data buffer
    ind = 0;
    memset(data, 0, sizeof(data));
    vTaskDelay(pdMS_TO_TICKS(TX_RATE_MS));
  }

  // The task will not reach this point as it runs indefinitely
  ESP_LOGI(TAG, "TX task ended");
  vTaskDelete(NULL);
}
void init_transmitter() {
  xTaskCreatePinnedToCore(transmitter_task, "transmitter_task", 4096, NULL, 20, NULL, 0);
}