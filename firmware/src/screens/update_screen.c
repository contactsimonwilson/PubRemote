#include "config.h"
#include "esp_log.h"
#include "remote/connection.h"
#include "remote/display.h"
#include "remote/espnow.h"
#include "remote/wifi.h"
#include "utilities/screen_utils.h"
#include "utilities/string_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <ui/ui.h>

static const char *TAG = "PUBREMOTE-UPDATE_SCREEN";

bool is_update_screen_active() {
  lv_obj_t *active_screen = lv_scr_act();
  return active_screen == ui_UpdateScreen;
}

void update_status_label() {
  char *formattedString;
  asprintf(&formattedString, "Wifi statuis: %s", wifi_get_connection_state_string());

  lv_label_set_text(ui_UpdateBodyLabel, formattedString);
  free(formattedString);
}

void update_task(void *pvParameters) {
  while (is_update_screen_active()) {
    if (LVGL_lock(-1)) {
      update_status_label();

      LVGL_unlock();
    }

    vTaskDelay(pdMS_TO_TICKS(LV_DISP_DEF_REFR_PERIOD));
  }

  ESP_LOGI(TAG, "Update task ended");
  vTaskDelete(NULL);
}

// Event handlers
void update_screen_load_start(lv_event_t *e) {
  ESP_LOGI(TAG, "Update screen load start");

  if (LVGL_lock(0)) {
    apply_ui_scale(NULL);
    create_navigation_group(ui_UpdateFooter);

    LVGL_unlock();
  }
}

void update_screen_loaded(lv_event_t *e) {
  ESP_LOGI(TAG, "Update screen loaded");
  connection_update_state(CONNECTION_STATE_DISCONNECTED);
  if (espnow_is_initialized()) {
    espnow_deinit();
  }
  if (!wifi_is_initialized()) {
    wifi_init();
  }

  // Start task to update UI
  xTaskCreate(update_task, "update_task", 4096, NULL, 2, NULL);
}

void update_screen_unload_start(lv_event_t *e) {
  if (wifi_is_initialized()) {
    wifi_uninit();
  }
  if (!espnow_is_initialized()) {
    espnow_init();
  }
  ESP_LOGI(TAG, "Update screen unload start");
  lv_obj_remove_event_cb(ui_AboutBody, paged_scroll_event_cb);
}

static void update_button_press2(lv_event_t *e) {
  ESP_LOGI(TAG, "Update button pressed");
  ESP_LOGI(TAG, "Current state: %s", wifi_get_connection_state_string());

  // Step 3: Scan for available networks
  wifi_network_info_t *networks = NULL;
  uint16_t network_count = 0;

  esp_err_t scan_ret = wifi_scan_networks(&networks, &network_count);
  if (scan_ret == ESP_OK && networks != NULL) {
    ESP_LOGI(TAG, "Processing %d scanned networks:", network_count);

    // Example: Find the strongest open network
    int strongest_open_rssi = -100;
    char strongest_open_ssid[33] = "";

    for (int i = 0; i < network_count; i++) {
      if (!networks[i].password_protected && networks[i].rssi > strongest_open_rssi) {
        strongest_open_rssi = networks[i].rssi;
        strncpy(strongest_open_ssid, networks[i].ssid, sizeof(strongest_open_ssid) - 1);
        strongest_open_ssid[sizeof(strongest_open_ssid) - 1] = '\0';
      }
    }

    if (strlen(strongest_open_ssid) > 0) {
      ESP_LOGI(TAG, "Strongest open network: %s (RSSI: %d)", strongest_open_ssid, strongest_open_rssi);
    }
    else {
      ESP_LOGI(TAG, "No open networks found");
    }

    // Example: List all networks with their security status
    ESP_LOGI(TAG, "All scanned networks:");
    for (int i = 0; i < network_count; i++) {
      ESP_LOGI(TAG, "  %s (RSSI: %d, %s)", networks[i].ssid, networks[i].rssi,
               networks[i].password_protected ? "Protected" : "Open");
    }

    // Don't forget to free the network list when done
    wifi_free_network_list(networks);
  }

  // Wait a bit before connecting
  vTaskDelay(pdMS_TO_TICKS(2000));

  // Step 4: Connect to a network (replace with your credentials)
  const char *ssid = "SiWi";
  const char *password = "internetgo";

  ESP_LOGI(TAG, "Auto-reconnect enabled: %s", wifi_is_auto_reconnect_enabled() ? "Yes" : "No");
  ESP_LOGI(TAG, "Current state: %s", wifi_get_connection_state_string());

  esp_err_t ret = wifi_connect_to_network(ssid, password);
  if (ret == ESP_OK) {
    ESP_LOGI(TAG, "Successfully connected to WiFi");
    ESP_LOGI(TAG, "Current state: %s", wifi_get_connection_state_string());

    // Demonstrate state monitoring during connection
    for (int i = 0; i < 20; i++) {
      ESP_LOGI(TAG, "State check %d: %s", i + 1, wifi_get_connection_state_string());
      vTaskDelay(pdMS_TO_TICKS(1000));

      // Simulate checking connection state in your application
      wifi_connection_state_t current_state = wifi_get_connection_state();
      if (current_state == WIFI_STATE_CONNECTED) {
        ESP_LOGI(TAG, "WiFi is connected - can proceed with network operations");

        // Example: You could start your network tasks here
        // xTaskCreate(http_client_task, "http_client", 4096, NULL, 5, NULL);
      }
      else if (current_state == WIFI_STATE_RECONNECTING) {
        ESP_LOGI(TAG, "WiFi is reconnecting - network operations should wait");
      }
    }

    // Step 5: Demonstrate manual disconnect
    ESP_LOGI(TAG, "Initiating manual disconnect...");
    wifi_disconnect();
    ESP_LOGI(TAG, "Current state: %s", wifi_get_connection_state_string());

    // Wait and monitor state after disconnect
    for (int i = 0; i < 5; i++) {
      vTaskDelay(pdMS_TO_TICKS(1000));
      ESP_LOGI(TAG, "Post-disconnect state: %s", wifi_get_connection_state_string());
    }
  }
  else {
    ESP_LOGE(TAG, "Failed to connect to WiFi");
    ESP_LOGI(TAG, "Current state: %s", wifi_get_connection_state_string());
  }

  // Step 6: Demonstrate auto-reconnect control
  ESP_LOGI(TAG, "Testing auto-reconnect control...");
  wifi_set_auto_reconnect(false);
  ESP_LOGI(TAG, "Auto-reconnect disabled");

  vTaskDelay(pdMS_TO_TICKS(1000));

  wifi_set_auto_reconnect(true);
  ESP_LOGI(TAG, "Auto-reconnect re-enabled");

  vTaskDelay(pdMS_TO_TICKS(1000));

  // Step 7: Final cleanup
  ESP_LOGI(TAG, "Starting WiFi cleanup...");
  wifi_uninit();
  ESP_LOGI(TAG, "Final state: %s", wifi_get_connection_state_string());

  ESP_LOGI(TAG, "WiFi example completed successfully");
}