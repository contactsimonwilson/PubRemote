#include "transmitter.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "peers.h"
#include "receiver.h"
#include "remoteinputs.h"
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

// Function to send ESP-NOW data
static void transmitter_task(void *pvParameters) {
  ESP_ERROR_CHECK(esp_now_register_send_cb(on_data_sent));
  ESP_LOGI(TAG, "Registered RX callback");

  ESP_LOGI(TAG, "TX task started");
  uint8_t combined_data[sizeof(int32_t) + sizeof(remote_data.bytes)];
  while (1) {
    int64_t newTime = get_current_time_ms();

    // Check if the last command was sent less than 1000ms ago
    if (newTime - LAST_COMMAND_TIME < COMMAND_TIMEOUT) {
      vTaskDelay(pdMS_TO_TICKS(TX_RATE_MS));
      continue;
    }
    if (pairing_settings.state == PAIRING_STATE_PAIRED) {
      // Create a new buffer to hold both secret_Code and remote_data.bytes
      // printf("Thumbstick x-axis value: %f\n", remote_data.data.js_x);
      // printf("Thumbstick y-axis value: %f\n", remote_data.data.js_y);
      // Copy secret_Code to the beginning of the buffer
      memcpy(combined_data, &pairing_settings.secret_code, sizeof(int32_t));

      // Copy remote_data.bytes after secret_Code
      memcpy(combined_data + sizeof(int32_t), remote_data.bytes, sizeof(remote_data.bytes));

      esp_err_t result = esp_now_send(&pairing_settings.remote_addr, combined_data, sizeof(combined_data));
      if (result != ESP_OK) {
        // Handle error if needed
        ESP_LOGE(TAG, "Error sending remote data: %d", result);
      }

      LAST_COMMAND_TIME = newTime;
      ESP_LOGD(TAG, "Sent command");
    }
    vTaskDelay(pdMS_TO_TICKS(TX_RATE_MS));
  }

  // The task will not reach this point as it runs indefinitely
  ESP_LOGI(TAG, "TX task ended");
  vTaskDelete(NULL);
}
void init_transmitter() {
  xTaskCreatePinnedToCore(transmitter_task, "transmitter_task", 4096, NULL, 20, NULL, 0);
}