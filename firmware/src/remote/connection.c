#include "connection.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "peers.h"
#include "receiver.h"
#include "remoteinputs.h"
#include "stats.h"
#include "time.h"
#include "transmitter.h"
#include <esp_err.h>
#include <esp_timer.h>
#include <remote/settings.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "PUBREMOTE-CONNECTION";
#define CONNECTION_TIMER_DELAY_MS 100
#define RECONNECTING_DURATION_MS 1000
#define TIMEOUT_DURATION_MS 30000

ConnectionState connection_state = CONNECTION_STATE_DISCONNECTED;
PairingState pairing_state = PAIRING_STATE_UNPAIRED;
static int64_t last_connection_state_change = 0;

void update_connection_state(ConnectionState state) {
  connection_state = state;
  last_connection_state_change = get_current_time_ms();
  update_stats_display();
}

// Use task rather than a timer so we can do heavy lifting in here
static void connection_task(void *pvParameters) {
  while (1) {
    if (connection_state == CONNECTION_STATE_DISCONNECTED) {
      // Do nothing
    }
    else if (connection_state == CONNECTION_STATE_CONNECTED) {
      if (get_current_time_ms() - remoteStats.lastUpdated > RECONNECTING_DURATION_MS) {
        // No data received for a while - update connection state
        update_connection_state(CONNECTION_STATE_RECONNECTING);
      }
    }
    else if (connection_state == CONNECTION_STATE_CONNECTING) {
      if (get_current_time_ms() - last_connection_state_change > TIMEOUT_DURATION_MS) {
        // Never connected - reset the connection state
        update_connection_state(CONNECTION_STATE_DISCONNECTED);
      }
      else if (remoteStats.lastUpdated > 0 &&
               get_current_time_ms() - remoteStats.lastUpdated < RECONNECTING_DURATION_MS) {
        // Connected - update connection state
        update_connection_state(CONNECTION_STATE_CONNECTED);
      }
    }
    else if (connection_state == CONNECTION_STATE_RECONNECTING) {
      if (get_current_time_ms() - last_connection_state_change > TIMEOUT_DURATION_MS) {
        // Reconnect failed - reset the connection state
        update_connection_state(CONNECTION_STATE_DISCONNECTED);
      }
      else if (get_current_time_ms() - remoteStats.lastUpdated < RECONNECTING_DURATION_MS) {
        // Reconnected - update connection state
        update_connection_state(CONNECTION_STATE_CONNECTED);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(CONNECTION_TIMER_DELAY_MS));
  }

  // The task will not reach this point as it runs indefinitely
  ESP_LOGI(TAG, "Connection management task ended");
  vTaskDelete(NULL);
}

void connect_to_peer(uint8_t *mac_addr, uint8_t channel) {
  esp_now_peer_info_t peerInfo = {};
  peerInfo.channel = channel; // Set the channel number (0-14)
  peerInfo.encrypt = false;
  memcpy(peerInfo.peer_addr, mac_addr, ESP_NOW_ETH_ALEN);

  if (esp_now_is_peer_exist(mac_addr)) {
    esp_err_t res = esp_now_del_peer(mac_addr);
    if (res != ESP_OK) {
      ESP_LOGE(TAG, "Failed to delete peer");
    }
  }

  esp_err_t result = esp_now_add_peer(&peerInfo);

  if (result == ESP_OK) {
    update_connection_state(CONNECTION_STATE_CONNECTING);
  }
  else {
    ESP_LOGE(TAG, "Failed to add peer");
  }
}

void connect_to_default_peer() {
  if (pairing_state == PAIRING_STATE_PAIRED) {
    uint8_t *mac_addr = pairing_settings.remote_addr;
    connect_to_peer(mac_addr, pairing_settings.channel);
  }
}

void init_connection() {
  if (pairing_settings.secret_code != DEFAULT_PAIRING_SECRET_CODE) {
    pairing_state = PAIRING_STATE_PAIRED;
  }

  // start off in connecting mode
  if (pairing_state == PAIRING_STATE_PAIRED) {
    connect_to_default_peer();
  }
  xTaskCreatePinnedToCore(connection_task, "connection_task", 2048, NULL, 20, NULL, 0);
}