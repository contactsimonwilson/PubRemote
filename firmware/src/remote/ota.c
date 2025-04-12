#include "ota.h"

static const char *TAG = "PUBMOTE-OTA";

static ota_state_t ota_state = {.in_progress = false,
                                .total_size = 0,
                                .received_size = 0,
                                .progress = 0,
                                .ota_handle = 0,
                                .update_partition = NULL};

// Function to handle the firmware upload path
static esp_err_t ota_update_handler(httpd_req_t *req) {
  ESP_LOGI(TAG, "OTA Update uploaded");
  char buf[FIRMWARE_BUFFER_SIZE];
  int total_received = 0;
  esp_err_t err;
  int content_length = req->content_len;
  int ret, remaining = req->content_len;

  ESP_LOGI(TAG, "Update request received, size: %d bytes", content_length);

  // Check if we already have an OTA in progress
  if (ota_state.in_progress) {
    ESP_LOGE(TAG, "OTA already in progres");
    return ESP_FAIL;
  }

  // Get the update partition
  ota_state.update_partition = esp_ota_get_next_update_partition(NULL);
  if (ota_state.update_partition == NULL) {
    ESP_LOGE(TAG, "Failed to get update partition");
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x", ota_state.update_partition->subtype,
           ota_state.update_partition->address);

  // Begin OTA
  err = esp_ota_begin(ota_state.update_partition, OTA_SIZE_UNKNOWN, &ota_state.ota_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to begin OTA update");
    return ESP_FAIL;
  }

  // Set OTA state
  ota_state.in_progress = true;
  ota_state.total_size = content_length;
  ota_state.received_size = 0;
  ota_state.progress = 0;

  // Process the uploaded firmware in chunks
  while (remaining > 0) {
    // Receive data
    // TODO - replace with some sort of esp-now read
    if ((ret = httpd_req_recv(req, buf, (remaining < FIRMWARE_BUFFER_SIZE ? remaining : FIRMWARE_BUFFER_SIZE) <= 0))) {
      if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
        // Retry receiving if timeout occurred
        continue;
      }

      // Handle error
      ESP_LOGE(TAG, "Failed to receive firmware data: %d", ret);
      esp_ota_abort(ota_state.ota_handle);
      ota_state.in_progress = false;

      const char *error_msg = "Failed to receive firmware data";
      httpd_resp_set_status(req, "500 Internal Server Error");
      httpd_resp_send(req, error_msg, strlen(error_msg));

      return ESP_FAIL;
    }

    // Write to flash
    err = esp_ota_write(ota_state.ota_handle, (const void *)buf, ret);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "Error writing to flash: %s", esp_err_to_name(err));
      esp_ota_abort(ota_state.ota_handle);
      ota_state.in_progress = false;
      return ESP_FAIL;
    }

    // Update progress
    remaining -= ret;
    total_received += ret;
    ota_state.received_size = total_received;
    ota_state.progress = (total_received * 100) / content_length;
    ESP_LOGI(TAG, "OTA Progress: %d%%", ota_state.progress);
  }

  // End OTA
  err = esp_ota_end(ota_state.ota_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error ending OTA update: %s", esp_err_to_name(err));
    ota_state.in_progress = false;
    return ESP_FAIL;
  }

  // Set boot partition
  err = esp_ota_set_boot_partition(ota_state.update_partition);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error setting boot partition: %s", esp_err_to_name(err));
    ota_state.in_progress = false;
    return ESP_FAIL;
  }

  // Update successful!
  ota_state.in_progress = false;
  ESP_LOGI(TAG, "OTA update successful! Rebooting...");

  // const char *success_msg = "Firmware update successful! Device will restart in a few seconds.";

  // Schedule restart task
  vTaskDelay(pdMS_TO_TICKS(2000)); // 2 seconds delay before restart
  esp_restart();

  return ESP_OK;
}

// Get the current OTA progress (0-100)
int ota_get_progress(void) {
  return ota_state.progress;
}

// Check if OTA is in progress
bool ota_is_updating(void) {
  return ota_state.in_progress;
}

void init_ota() {
}

void teardown_ota() {
}