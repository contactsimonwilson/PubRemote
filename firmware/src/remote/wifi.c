#include "wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "settings.h"

static const char *TAG = "PUBMOTE-WIFI_MANAGER";

// Event group to signal when WiFi is connected
static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

// Current WiFi operation mode
static wifi_operation_mode_t current_mode = WIFI_MODE_ESPNOW;

// WiFi configuration
static wifi_config_t wifi_sta_config = {
    .sta =
        {
            .ssid = "YOUR_SSID",
            .password = "YOUR_PASSWORD",
        },
};

static wifi_config_t wifi_ap_config = {
    .ap = {.ssid = "ESP_OTA_AP", .password = "password123", .max_connection = 4, .authmode = WIFI_AUTH_WPA_WPA2_PSK},
};

// Event handler for WiFi events
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT) {
    switch (event_id) {
    case WIFI_EVENT_STA_START:
      esp_wifi_connect();
      break;
    case WIFI_EVENT_STA_CONNECTED:
      ESP_LOGI(TAG, "Connected to WiFi");
      break;
    case WIFI_EVENT_STA_DISCONNECTED:
      ESP_LOGI(TAG, "Disconnected from WiFi, retrying...");
      esp_wifi_connect();
      break;
    case WIFI_EVENT_AP_STACONNECTED:
      ESP_LOGI(TAG, "Station connected to AP");
      break;
    case WIFI_EVENT_AP_STADISCONNECTED:
      ESP_LOGI(TAG, "Station disconnected from AP");
      break;
    default:
      break;
    }
  }
  else if (event_base == IP_EVENT) {
    if (event_id == IP_EVENT_STA_GOT_IP) {
      ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
      ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
      xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
  }
}

static esp_err_t init_common() {
  // Initialize WiFi in station mode for ESP-NOW
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  // ESP_ERROR_CHECK(esp_event_loop_create_default()); // Remove this line
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  return ESP_OK;
}

// Initialize ESP-NOW
static esp_err_t init_espnow() {
  ESP_LOGI(TAG, "Initializing ESP-NOW");
  init_common();
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());
  // Initialize ESP-NOW
  ESP_ERROR_CHECK(esp_now_init());
  // Stop scanning for WiFi networks
  ESP_ERROR_CHECK(esp_wifi_scan_stop());
  ESP_ERROR_CHECK(esp_wifi_set_channel(pairing_settings.channel, WIFI_SECOND_CHAN_NONE));

  ESP_LOGI(TAG, "ESP-NOW initialized");
  return ESP_OK;
}

// Deinitialize ESP-NOW
static esp_err_t deinit_espnow() {
  // Unregister ESP-NOW peers if needed
  // esp_now_del_peer(...);

  // Deinit ESP-NOW
  esp_err_t err = esp_now_deinit();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "ESP-NOW deinit failed: %s", esp_err_to_name(err));
    return err;
  }

  ESP_LOGI(TAG, "ESP-NOW deinitialized");
  return ESP_OK;
}

// Initialize WiFi in station mode for OTA
static esp_err_t init_wifi_sta_mode(void) {
  ESP_LOGI(TAG, "Initializing WiFi station mode for OTA");
  init_common();
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "WiFi station mode initialized, connecting to %s", wifi_sta_config.sta.ssid);

  // Wait for connection (with timeout)
  EventBits_t bits = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, pdMS_TO_TICKS(10000));

  if (bits & WIFI_CONNECTED_BIT) {
    ESP_LOGI(TAG, "Connected to WiFi");
    return ESP_OK;
  }
  else {
    ESP_LOGE(TAG, "Failed to connect to WiFi");
    return ESP_FAIL;
  }
}

// Initialize WiFi in AP mode for OTA
static esp_err_t init_wifi_ap_mode(void) {
  ESP_LOGI(TAG, "Initializing WiFi AP mode for OTA");
  init_common();
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "WiFi AP mode initialized, SSID: %s", wifi_ap_config.ap.ssid);
  return ESP_OK;
}

// Stop WiFi
static esp_err_t stop_wifi() {
  ESP_LOGI(TAG, "Stopping WiFi");

  esp_err_t err = esp_wifi_stop();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to stop WiFi: %s", esp_err_to_name(err));
    return err;
  }

  err = esp_wifi_deinit();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to deinit WiFi: %s", esp_err_to_name(err));
    return err;
  }

  ESP_LOGI(TAG, "WiFi stopped successfully");
  return ESP_OK;
}

// Change WiFi operation mode
esp_err_t change_wifi_mode(wifi_operation_mode_t new_mode) {
  ESP_LOGI(TAG, "Changing WiFi mode from %d to %d", current_mode, new_mode);

  if (current_mode == new_mode) {
    ESP_LOGI(TAG, "Already in requested mode");
    return ESP_OK;
  }

  // Step 1: Deinitialize current mode
  if (current_mode == WIFI_MODE_ESPNOW) {
    // Deinitialize ESP-NOW
    ESP_ERROR_CHECK(deinit_espnow());
  }

  // Step 2: Stop WiFi completely
  ESP_ERROR_CHECK(stop_wifi());

  // Let the system stabilize after stopping WiFi
  vTaskDelay(pdMS_TO_TICKS(500));

  // Step 3: Initialize new mode
  esp_err_t result = ESP_OK;

  switch (new_mode) {
  case WIFI_MODE_ESPNOW:
    result = init_espnow();
    break;

  case WIFI_MODE_OTA_STA:
    result = init_wifi_sta_mode();
    break;

  case WIFI_MODE_OTA_AP:
    result = init_wifi_ap_mode();
    break;

  default:
    ESP_LOGE(TAG, "Unknown WiFi mode requested");
    return ESP_FAIL;
  }

  if (result == ESP_OK) {
    current_mode = new_mode;
    ESP_LOGI(TAG, "Successfully changed to new WiFi mode: %d", new_mode);
  }
  else {
    ESP_LOGE(TAG, "Failed to change to new WiFi mode, attempting to revert to ESP-NOW");
    // Attempt to go back to ESP-NOW mode
    init_espnow();
    current_mode = WIFI_MODE_ESPNOW;
  }

  return result;
}

// Initialize the WiFi mode manager
esp_err_t init_wifi() {
  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // Initialize TCP/IP stack
  ESP_ERROR_CHECK(esp_netif_init());

  // Create WiFi event group
  wifi_event_group = xEventGroupCreate();

  // Register event handlers
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

  // Start in ESP-NOW mode by default
  return init_espnow();
}