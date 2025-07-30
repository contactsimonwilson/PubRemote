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

static lv_obj_t *kb = NULL;
static lv_obj_t *password_field = NULL;

typedef enum {
  UPDATE_STEP_START,
  UPDATE_SCANNING,
  UPDATE_STEP_SELECT_NETWORK,
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

bool is_update_screen_active() {
  lv_obj_t *active_screen = lv_scr_act();
  return active_screen == ui_UpdateScreen;
}

void update_status_label(bool step_has_changed) {
  switch (current_update_step) {
  case UPDATE_STEP_START:
    if (step_has_changed) {
      lv_label_set_text(ui_UpdateHeaderLabel, "Update");
      lv_label_set_text(ui_UpdateBodyLabel, "Click next to scan for networks");
    }
    // Add button to footer
    break;
  case UPDATE_SCANNING:
    if (step_has_changed) {
      lv_label_set_text(ui_UpdateHeaderLabel, "Scanning");
      lv_label_set_text(ui_UpdateBodyLabel, "Scanning for networks...");
    }
    break;
  case UPDATE_STEP_SELECT_NETWORK:
    if (step_has_changed) {
      lv_label_set_text(ui_UpdateHeaderLabel, "Networks");
      lv_label_set_text(ui_UpdateBodyLabel, "Select a network to connect");
    }
    break;
  case UPDATE_STEP_ENTER_PASSWORD:
    if (step_has_changed) {
      lv_label_set_text(ui_UpdateHeaderLabel, "Authentication");
      lv_label_set_text(ui_UpdateBodyLabel, "Enter password for the selected network");
    }
    // Add keyboard input for password
    break;
  case UPDATE_STEP_CONNECTING:
    if (step_has_changed) {
      lv_label_set_text(ui_UpdateHeaderLabel, "Connecting");
      lv_label_set_text(ui_UpdateBodyLabel, "Connecting to the network...");
    }
    break;
  case UPDATE_STEP_CHECKING_UPDATE:
    if (step_has_changed) {
      lv_label_set_text(ui_UpdateHeaderLabel, "Checking");
      lv_label_set_text(ui_UpdateBodyLabel, "Checking for updates...");
    }
    break;
  case UPDATE_STEP_UPDATE_AVAILABLE:
    if (step_has_changed) {
      lv_label_set_text(ui_UpdateHeaderLabel, "Update Available");
      lv_label_set_text(ui_UpdateBodyLabel, "Update available! Click next to download.");
    }
    break;
  case UPDATE_STEP_NO_UPDATE:
    if (step_has_changed) {
      lv_label_set_text(ui_UpdateHeaderLabel, "No Update");
      lv_label_set_text(ui_UpdateBodyLabel, "No updates available.");
    }
    break;
  case UPDATE_STEP_DOWNLOADING:
    if (step_has_changed) {
      lv_label_set_text(ui_UpdateHeaderLabel, "Download");
      lv_label_set_text(ui_UpdateBodyLabel, "Downloading update...");
    }
    break;
  case UPDATE_STEP_VALIDATE:
    if (step_has_changed) {
      lv_label_set_text(ui_UpdateHeaderLabel, "Validation");
      lv_label_set_text(ui_UpdateBodyLabel, "Validating downloaded update...");
    }
    break;
  case UPDATE_STEP_INSTALLING:
    if (step_has_changed) {
      lv_label_set_text(ui_UpdateHeaderLabel, "Installing");
      lv_label_set_text(ui_UpdateBodyLabel, "Installing update...");
    }
    break;
  case UPDATE_STEP_COMPLETE:
    if (step_has_changed) {
      lv_label_set_text(ui_UpdateHeaderLabel, "Complete");
      lv_label_set_text(ui_UpdateBodyLabel, "Update complete! Restarting...");
    }
    // Restart the device after a short delay
    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();
    break;
  case UPDATE_STEP_ERROR:
    lv_label_set_text(ui_UpdateHeaderLabel, "Error");
    lv_label_set_text(ui_UpdateBodyLabel, "An error occurred during the update process.");
    // Optionally, you could add a retry button here
    break;
  default:
    lv_label_set_text(ui_UpdateBodyLabel, "Unknown update step.");
    break;
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

    if (LVGL_lock(-1)) {
      update_status_label(step_has_changed);

      LVGL_unlock();
    }

    last_step = current_update_step;

    vTaskDelay(pdMS_TO_TICKS(LV_DISP_DEF_REFR_PERIOD));
  }

  ESP_LOGI(TAG, "Update task ended");
  vTaskDelete(NULL);
  update_task_handle = NULL;
}

static void ta_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *ta = lv_event_get_target(e);
  lv_obj_t *kb = (lv_obj_t *)lv_event_get_user_data(e);
  if (code == LV_EVENT_FOCUSED) {
    lv_keyboard_set_textarea(kb, ta);
    lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
  }

  if (code == LV_EVENT_DEFOCUSED) {
    lv_keyboard_set_textarea(kb, NULL);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
  }
}

// Event handlers
void update_screen_load_start(lv_event_t *e) {
  ESP_LOGI(TAG, "Update screen load start");
  current_update_step = UPDATE_STEP_START;

  if (LVGL_lock(0)) {
    apply_ui_scale(NULL);
    create_navigation_group(ui_UpdateFooter);

    // /*Create a keyboard to use it with an of the text areas*/
    // lv_obj_t *kb = lv_keyboard_create(lv_scr_act());
    // lv_obj_set_size(kb, LV_PCT(80), 100);

    // /*Create a text area. The keyboard will write here*/
    // lv_obj_t *ta1;
    // ta1 = lv_textarea_create(ui_UpdateBody);
    // lv_obj_align(ta1, LV_ALIGN_CENTER, 10, 10);
    // lv_obj_add_event_cb(ta1, ta_event_cb, LV_EVENT_ALL, kb);
    // lv_textarea_set_placeholder_text(ta1, "Enter wifi password");
    // lv_obj_set_size(kb, LV_PCT(100), 32);

    // // lv_obj_t *ta2;
    // // ta2 = lv_textarea_create(lv_scr_act());
    // // lv_obj_align(ta2, LV_ALIGN_TOP_RIGHT, -10, 10);
    // // lv_obj_add_event_cb(ta2, ta_event_cb, LV_EVENT_ALL, kb);
    // // lv_obj_set_size(ta2, 140, 80);

    // lv_keyboard_set_textarea(kb, ta1);

    LVGL_unlock();
  }
}

void update_screen_loaded(lv_event_t *e) {
  ESP_LOGI(TAG, "Update screen loaded");

  // Start task to update UI
  // update_task_handle = xTaskCreate(update_task, "update_task", 4096, NULL, 2, NULL);
  xTaskCreate(update_task, "update_task", 4096, NULL, 2, NULL);
}

void update_screen_unload_start(lv_event_t *e) {
  // if (update_task_handle != NULL) {
  //   vTaskDelete(update_task_handle);
  //   update_task_handle = NULL;
  // }
  if (wifi_is_initialized()) {
    wifi_uninit();
  }
  if (!espnow_is_initialized()) {
    espnow_init();
  }
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
  current_update_step++;
}

void update_secondary_button_press(lv_event_t *e) {
  // Screen switch is already handled by the squareline studio generated code
  // _ui_screen_change(&ui_MenuScreen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, &ui_MenuScreen_screen_init);
}