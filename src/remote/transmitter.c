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
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
static const char *TAG = "PUBREMOTE-TRANSMITTER";

// Function to send ESP-NOW data
static void transmitter_task(void *pvParameters) {
  ESP_LOGI(TAG, "TX task started");
  uint8_t combined_data[sizeof(int32_t) + sizeof(remote_data.bytes)];
  while (1) {
    int64_t newTime = get_current_time_ms();

// Check if the last command was sent less than 1000ms ago
#define COMMAND_TIMEOUT 1000
    if (newTime - LAST_COMMAND_TIME < COMMAND_TIMEOUT) {
      vTaskDelay(TRANSMIT_FREQUENCY);
      continue;
    }
    if (pairing_state == PAIRING_STATE_PAIRED) {
      // Create a new buffer to hold both secret_Code and remote_data.bytes
      // printf("Thumbstick x-axis value: %f\n", remote_data.data.js_x);
      // printf("Thumbstick y-axis value: %f\n", remote_data.data.js_y);
      // Copy secret_Code to the beginning of the buffer
      memcpy(combined_data, &secret_code, sizeof(int32_t));

      // Copy remote_data.bytes after secret_Code
      memcpy(combined_data + sizeof(int32_t), remote_data.bytes, sizeof(remote_data.bytes));

      esp_err_t result = esp_now_send(&remote_addr, combined_data, sizeof(combined_data));
      if (result != ESP_OK) {
        // Handle error if needed
        ESP_LOGE(TAG, "Error sending data: %d", result);
      }

      LAST_COMMAND_TIME = newTime;
      ESP_LOGI(TAG, "Sent command");
    }
    vTaskDelay(TRANSMIT_FREQUENCY);
  }

  // The task will not reach this point as it runs indefinitely
  ESP_LOGI(TAG, "TX task ended");
  vTaskDelete(NULL);
}

static void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  ESP_LOGI(TAG, "SENT");
}

void init_transmitter() {
  ESP_LOGI(TAG, "Registered RX callback. Creating tasks");
  ESP_ERROR_CHECK(esp_now_register_send_cb(on_data_sent));
  xTaskCreate(transmitter_task, "transmitter_task", 4096, NULL, 2, NULL);
}