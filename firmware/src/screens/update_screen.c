#include "config.h"
#include "esp_log.h"
#include "lvgl_elements/password_input.h"
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

typedef enum {
  UPDATE_STEP_START,
  UPDATE_SCANNING,
  UPDATE_STEP_ENTER_PASSWORD,
  UPDATE_STEP_CONNECTING,
  UPDATE_STEP_CHECKING_UPDATE,
  UPDATE_STEP_UPDATE_AVAILABLE,
  UPDATE_STEP_NO_UPDATE,
  UPDATE_STEP_DOWNLOADING,
  UPDATE_STEP_VALIDATE,
  UPDATE_STEP_INSTALLING,
  UPDATE_STEP_COMPLETE,
  UPDATE_STEP_ERROR
} UpdateStep;

static UpdateStep current_update_step = UPDATE_STEP_START;
static TaskHandle_t update_task_handle = NULL;
static wifi_network_info_t *networks = NULL;
static uint16_t network_count = 0;
static char selected_network_ssid[33] = {0}; // Fixed: Use array instead of pointer
static TickType_t last_scan_time = 0;
static const TickType_t SCAN_INTERVAL = pdMS_TO_TICKS(3000); // Scan every 3 seconds

bool is_update_screen_active() {
  lv_obj_t *active_screen = lv_scr_act();
  return active_screen == ui_UpdateScreen;
}

void update_status_label() {
  switch (current_update_step) {
  case UPDATE_STEP_START:
    lv_label_set_text(ui_UpdateHeaderLabel, "Update");
    lv_label_set_text(ui_UpdateBodyLabel, "Click next to scan for networks");
    lv_obj_clear_flag(ui_UpdateBodyLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_flex_align(ui_UpdateBody, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    break;
  case UPDATE_SCANNING:
    lv_label_set_text(ui_UpdateHeaderLabel, "Network Scan");
    lv_label_set_text(ui_UpdatePrimaryActionButtonLabel, "Stop");
    lv_label_set_text(ui_UpdateBodyLabel, "Scanning...");
    lv_obj_set_flex_align(ui_UpdateBody, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    break;
  case UPDATE_STEP_ENTER_PASSWORD:
    lv_label_set_text(ui_UpdateHeaderLabel, "Authentication");
    lv_label_set_text_fmt(ui_UpdateBodyLabel, "Enter password for  %s", selected_network_ssid);
    lv_label_set_text(ui_UpdatePrimaryActionButtonLabel, "Next");
    lv_obj_clear_flag(ui_UpdateBodyLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_flex_align(ui_UpdateBody, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    create_wifi_password_screen(ui_UpdateBody);
    break;
  case UPDATE_STEP_CONNECTING:
    lv_label_set_text(ui_UpdateHeaderLabel, "Connecting");
    lv_label_set_text(ui_UpdateBodyLabel, "Connecting to the network...");
    lv_label_set_text(ui_UpdatePrimaryActionButtonLabel, "Stop");
    lv_obj_clear_flag(ui_UpdateBodyLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_flex_align(ui_UpdateBody, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    break;
  case UPDATE_STEP_CHECKING_UPDATE:
    lv_label_set_text(ui_UpdateHeaderLabel, "Checking");
    lv_label_set_text(ui_UpdateBodyLabel, "Checking for updates...");
    lv_label_set_text(ui_UpdatePrimaryActionButtonLabel, "Stop");
    lv_obj_clear_flag(ui_UpdateBodyLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_flex_align(ui_UpdateBody, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    break;
  case UPDATE_STEP_UPDATE_AVAILABLE:
    lv_label_set_text(ui_UpdateHeaderLabel, "Update Available");
    lv_label_set_text(ui_UpdateBodyLabel, "Update available! Click next to download.");
    lv_label_set_text(ui_UpdatePrimaryActionButtonLabel, "Next");
    lv_obj_clear_flag(ui_UpdateBodyLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_flex_align(ui_UpdateBody, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    break;
  case UPDATE_STEP_NO_UPDATE:
    lv_label_set_text(ui_UpdateHeaderLabel, "No Update");
    lv_label_set_text(ui_UpdateBodyLabel, "No updates available.");
    lv_label_set_text(ui_UpdatePrimaryActionButtonLabel, "Exit");
    lv_obj_clear_flag(ui_UpdateBodyLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_flex_align(ui_UpdateBody, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    break;
  case UPDATE_STEP_DOWNLOADING:
    lv_label_set_text(ui_UpdateHeaderLabel, "Download");
    lv_label_set_text(ui_UpdateBodyLabel, "Downloading update...");
    lv_label_set_text(ui_UpdatePrimaryActionButtonLabel, "Stop");
    lv_obj_clear_flag(ui_UpdateBodyLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_flex_align(ui_UpdateBody, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    break;
  case UPDATE_STEP_VALIDATE:
    lv_label_set_text(ui_UpdateHeaderLabel, "Validation");
    lv_label_set_text(ui_UpdateBodyLabel, "Validating downloaded update...");
    lv_label_set_text(ui_UpdatePrimaryActionButtonLabel, "Stop");
    lv_obj_clear_flag(ui_UpdateBodyLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_flex_align(ui_UpdateBody, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    break;
  case UPDATE_STEP_INSTALLING:
    lv_label_set_text(ui_UpdateHeaderLabel, "Installing");
    lv_label_set_text(ui_UpdateBodyLabel, "Installing update...");
    lv_label_set_text(ui_UpdatePrimaryActionButtonLabel, "Stop");
    lv_obj_clear_flag(ui_UpdateBodyLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_flex_align(ui_UpdateBody, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    break;
  case UPDATE_STEP_COMPLETE:
    lv_label_set_text(ui_UpdateHeaderLabel, "Complete");
    lv_label_set_text(ui_UpdateBodyLabel, "Update complete! Restarting...");
    lv_label_set_text(ui_UpdatePrimaryActionButtonLabel, "Reboot");
    lv_obj_clear_flag(ui_UpdateBodyLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_flex_align(ui_UpdateBody, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    break;
  case UPDATE_STEP_ERROR:
    lv_label_set_text(ui_UpdateHeaderLabel, "Error");
    lv_label_set_text(ui_UpdateBodyLabel, "An error occurred during the update process.");
    lv_label_set_text(ui_UpdatePrimaryActionButtonLabel, "Retry");
    lv_obj_clear_flag(ui_UpdateBodyLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_flex_align(ui_UpdateBody, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    break;
  default:
    lv_label_set_text(ui_UpdateBodyLabel, "Unknown update step.");
    lv_obj_clear_flag(ui_UpdateBodyLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_flex_align(ui_UpdateBody, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    break;
  }
}

static void clean_body() {
  // Iterate backwards to avoid index shifting issues when deleting
  uint32_t child_cnt = lv_obj_get_child_cnt(ui_UpdateBody);
  for (int32_t i = child_cnt - 1; i >= 0; i--) {
    lv_obj_t *child = lv_obj_get_child(ui_UpdateBody, i);
    if (child != ui_UpdateBodyLabel && child != NULL) {
      lv_obj_del(child);
    }
  }
}

static void network_button_clicked(lv_event_t *e) {
  char *ssid = (char *)lv_event_get_user_data(e);
  ESP_LOGI(TAG, "Selected network: %s", ssid);

  strncpy(selected_network_ssid, ssid, sizeof(selected_network_ssid) - 1);
  selected_network_ssid[sizeof(selected_network_ssid) - 1] = '\0';

  free(ssid);

  current_update_step = UPDATE_STEP_ENTER_PASSWORD;

  // Force immediate UI update
  if (LVGL_lock(0)) {
    clean_body();          // Clear the network buttons
    update_status_label(); // Update the UI immediately
    LVGL_unlock();
  }
}

void update_task(void *pvParameters) {
  connection_update_state(CONNECTION_STATE_DISCONNECTED);
  if (espnow_is_initialized()) {
    espnow_deinit();
  }
  if (!wifi_is_initialized()) {
    wifi_init();
  }

  UpdateStep last_step = current_update_step;
  bool is_first_run = true;

  while (is_update_screen_active()) {
    bool step_has_changed = (current_update_step != last_step || is_first_run);
    if (step_has_changed) {
      is_first_run = false;
      ESP_LOGI(TAG, "Update step changed: %d", current_update_step);
    }

    if (step_has_changed) {
      if (LVGL_lock(0)) {
        update_status_label();
        LVGL_unlock();
      }
    }

    switch (current_update_step) {
    case UPDATE_STEP_START:
      break;
    case UPDATE_SCANNING:
      // Continuous scanning with interval control
      TickType_t current_time = xTaskGetTickCount();
      if (step_has_changed || (current_time - last_scan_time) >= SCAN_INTERVAL) {
        ESP_LOGI(TAG, "Network count: %d", network_count);

        // Free previous scan results
        if (networks != NULL) {
          wifi_free_network_list(networks);
          networks = NULL;
          network_count = 0;
        }

        // Start scanning for networks
        esp_err_t scan_ret = wifi_scan_networks(&networks, &network_count);
        if (scan_ret == ESP_OK && networks != NULL && current_update_step == UPDATE_SCANNING) {
          ESP_LOGI(TAG, "Processing %d scanned networks:", network_count);

          // Sort by highest RSSI first
          for (int i = 0; i < network_count - 1; i++) {
            for (int j = i + 1; j < network_count; j++) {
              if (networks[i].rssi < networks[j].rssi) {
                wifi_network_info_t temp = networks[i];
                networks[i] = networks[j];
                networks[j] = temp;
              }
            }
          }

          if (LVGL_lock(0)) {
            clean_body();

            // Add buttons with details of each network
            for (int i = 0; i < network_count; i++) {
              lv_obj_t *network_button = lv_btn_create(ui_UpdateBody);
              lv_obj_set_size(network_button, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
              lv_obj_set_style_bg_color(network_button, lv_color_hex(0xFFFFFF), 0);
              lv_obj_set_width(network_button, lv_pct(100));
              lv_obj_set_style_border_color(network_button, lv_color_hex(0xCCCCCC), 0);
              lv_obj_set_style_border_width(network_button, 1, 0);
              lv_obj_set_style_radius(network_button, 5, 0);
              lv_obj_set_style_pad_all(network_button, 5, 0);

              lv_obj_t *network_label = lv_label_create(network_button);
              lv_label_set_text_fmt(network_label, "%s (%d dBm)", networks[i].ssid, networks[i].rssi);
              lv_obj_set_style_text_color(network_label, lv_color_hex(0x000000), 0);

              char *ssid_copy = malloc(strlen(networks[i].ssid) + 1);
              strcpy(ssid_copy, networks[i].ssid);
              lv_obj_add_event_cb(network_button, network_button_clicked, LV_EVENT_CLICKED, ssid_copy);
              lv_obj_align(network_button, LV_ALIGN_TOP_MID, 0, i * 40);
            }

            if (network_count > 0) {
              lv_obj_add_flag(ui_UpdateBodyLabel, LV_OBJ_FLAG_HIDDEN);
              lv_obj_set_flex_align(ui_UpdateBody, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            }
            else {
              lv_obj_clear_flag(ui_UpdateBodyLabel, LV_OBJ_FLAG_HIDDEN);
              lv_obj_set_flex_align(ui_UpdateBody, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            }
            LVGL_unlock();
          }
        }
        last_scan_time = current_time;
      }
      break;
    case UPDATE_STEP_ENTER_PASSWORD:
      break;
    case UPDATE_STEP_CONNECTING:
      // Start connecting
      break;
    case UPDATE_STEP_CHECKING_UPDATE:
      // Check for updates
      break;
    case UPDATE_STEP_UPDATE_AVAILABLE:
      break;
    case UPDATE_STEP_NO_UPDATE:
      break;
    case UPDATE_STEP_DOWNLOADING:
      // Start downloading update
      break;
    case UPDATE_STEP_VALIDATE:
      // Validate downloaded update
      break;
    case UPDATE_STEP_INSTALLING:
      // Start installing update
      break;
    case UPDATE_STEP_COMPLETE:
      break;
    case UPDATE_STEP_ERROR:
      // Handle error case
      break;
    }

    last_step = current_update_step;
    vTaskDelay(pdMS_TO_TICKS(LV_DISP_DEF_REFR_PERIOD));
  }

  // Cleanup
  if (wifi_is_initialized()) {
    wifi_uninit();
  }
  if (!espnow_is_initialized()) {
    espnow_init();
  }

  if (networks != NULL) {
    wifi_free_network_list(networks);
    networks = NULL;
    network_count = 0;
  }

  ESP_LOGI(TAG, "Update task ended");
  vTaskDelete(NULL);
  update_task_handle = NULL;
}

// Event handlers
void update_screen_load_start(lv_event_t *e) {
  ESP_LOGI(TAG, "Update screen load start");
  current_update_step = UPDATE_STEP_START;

  if (LVGL_lock(0)) {
    apply_ui_scale(NULL);
    create_navigation_group(ui_UpdateFooter);
    LVGL_unlock();
  }
}

void update_screen_loaded(lv_event_t *e) {
  ESP_LOGI(TAG, "Update screen loaded");
  xTaskCreate(update_task, "update_task", 4096, NULL, 2, NULL);
}

void update_screen_unload_start(lv_event_t *e) {
  ESP_LOGI(TAG, "Update screen unload start");
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

void update_primary_button_press(lv_event_t *e) {
  switch (current_update_step) {
  case UPDATE_STEP_START:
    current_update_step = UPDATE_SCANNING;
    break;
  case UPDATE_STEP_ENTER_PASSWORD:
    current_update_step = UPDATE_STEP_CONNECTING;
    break;
  case UPDATE_STEP_CONNECTING:
    current_update_step = UPDATE_STEP_CHECKING_UPDATE;
    break;
  case UPDATE_STEP_CHECKING_UPDATE:
    current_update_step = UPDATE_STEP_UPDATE_AVAILABLE;
    break;
  case UPDATE_STEP_UPDATE_AVAILABLE:
    current_update_step = UPDATE_STEP_DOWNLOADING;
    break;
  case UPDATE_STEP_NO_UPDATE:
    if (LVGL_lock(0)) {
      _ui_screen_change(&ui_MenuScreen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, &ui_MenuScreen_screen_init);
      LVGL_unlock();
    }
    break;
  case UPDATE_STEP_DOWNLOADING:
    current_update_step = UPDATE_STEP_UPDATE_AVAILABLE;
    break;
  case UPDATE_STEP_VALIDATE:
    current_update_step = UPDATE_STEP_UPDATE_AVAILABLE;
    break;
  case UPDATE_STEP_INSTALLING:
    current_update_step = UPDATE_STEP_UPDATE_AVAILABLE;
    break;
  case UPDATE_STEP_COMPLETE:
    // Set timer for 5 second delay before restart
    break;
  case UPDATE_STEP_ERROR:
    current_update_step = UPDATE_STEP_START;
    break;
  default:
    ESP_LOGE(TAG, "Unknown update step: %d", current_update_step);
    current_update_step = UPDATE_STEP_START;
    break;
  }
}

void update_secondary_button_press(lv_event_t *e) {
  // Screen switch is already handled by the squareline studio generated code
}