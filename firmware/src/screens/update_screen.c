#include "config.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "ota/release_client.h"
#include "remote/connection.h"
#include "remote/display.h"
#include "remote/espnow.h"
#include "remote/settings.h"
#include "remote/wifi.h"
#include "utilities/screen_utils.h"
#include "utilities/string_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <ui/ui.h>

static const char *TAG = "PUBREMOTE-UPDATE_SCREEN";

#define FORCE_UPDATE 1

typedef enum {
  UPDATE_STEP_START,
  UPDATE_STEP_CONNECTING,
  UPDATE_STEP_CHECKING_UPDATE,
  UPDATE_STEP_UPDATE_AVAILABLE,
  UPDATE_STEP_NO_UPDATE,
  UPDATE_STEP_DOWNLOADING,
  UPDATE_STEP_VALIDATE,
  UPDATE_STEP_INSTALLING,
  UPDATE_STEP_COMPLETE,
  UPDATE_STEP_NO_WIFI,
  UPDATE_STEP_ERROR
} UpdateStep;

typedef enum {
  UPDATE_TYPE_STABLE,
  UPDATE_TYPE_PRERELEASE,
  UPDATE_TYPE_NIGHTLY
} UpdateType;

typedef struct {
  UpdateType type;
  char tag_name[32];
  char name[64];
  char download_url[256];
} ReleaseInfo;

static UpdateStep current_update_step = UPDATE_STEP_START;
static TaskHandle_t update_task_handle = NULL;
static ReleaseInfo available_updates[3]; // Stable, Prerelease, Nightly
static int available_update_count = 0;
static UpdateType selected_update_type = UPDATE_TYPE_STABLE;

bool is_update_screen_active() {
  lv_obj_t *active_screen = lv_scr_act();
  return active_screen == ui_UpdateScreen;
}

static void change_update_selection(lv_event_t *e) {
  lv_obj_t *obj = lv_event_get_target(e);
  int selected = lv_dropdown_get_selected(obj);

  if (selected < 0 || selected >= available_update_count) {
    ESP_LOGW(TAG, "Invalid update selection index: %d", selected);
    return;
  }
  selected_update_type = available_updates[selected].type;
  ESP_LOGI(TAG, "Selected update type: %d", selected_update_type);
}

static void update_status_label() {
  ESP_LOGI(TAG, "Updating status label for step %d", current_update_step);
  static char *wifi_ssid;
  switch (current_update_step) {
  case UPDATE_STEP_START:
    wifi_ssid = get_wifi_ssid();
    lv_label_set_text(ui_UpdateHeaderLabel, "Update");
    lv_label_set_text_fmt(ui_UpdateBodyLabel, "Click next to connect to %s", wifi_ssid);
    lv_obj_clear_flag(ui_UpdateBodyLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui_UpdatePrimaryActionButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_flex_align(ui_UpdateBody, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    break;
  case UPDATE_STEP_CONNECTING:
    wifi_ssid = get_wifi_ssid();
    lv_label_set_text(ui_UpdateHeaderLabel, "Connecting");
    lv_label_set_text_fmt(ui_UpdateBodyLabel, "Connecting to %s...", wifi_ssid);
    lv_obj_clear_flag(ui_UpdateBodyLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_UpdatePrimaryActionButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_flex_align(ui_UpdateBody, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    break;
  case UPDATE_STEP_CHECKING_UPDATE:
    lv_label_set_text(ui_UpdateHeaderLabel, "Checking");
    lv_label_set_text(ui_UpdateBodyLabel, "Checking for updates...");
    lv_obj_add_flag(ui_UpdatePrimaryActionButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui_UpdateBodyLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_UpdatePrimaryActionButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_flex_align(ui_UpdateBody, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    break;
  case UPDATE_STEP_UPDATE_AVAILABLE:
    lv_label_set_text(ui_UpdateHeaderLabel, "Update Available");
    lv_label_set_text(ui_UpdateBodyLabel, "Choose from available updates");
    lv_label_set_text(ui_UpdatePrimaryActionButtonLabel, "Next");
    // Add lvgl dropdown
    lv_obj_t *update_dropdown = lv_dropdown_create(ui_UpdateBody);
    char *available_options = strdup("");

    for (int i = 0; i < available_update_count; i++) {
      char *old = available_options;

      if (asprintf(&available_options, "%s%s%s", old,
                   (i > 0) ? "\n" : "", // Add newline if not first
                   available_updates[i].name) == -1) {
        free(old);
        return;
      }

      free(old);
    }

    lv_dropdown_set_options(update_dropdown, available_options);
    free(available_options);
    lv_obj_set_width(update_dropdown, lv_pct(100));
    lv_obj_set_height(update_dropdown, LV_SIZE_CONTENT); /// 1
    lv_obj_set_align(update_dropdown, LV_ALIGN_CENTER);
    lv_obj_add_flag(update_dropdown, LV_OBJ_FLAG_SCROLL_ON_FOCUS);  /// Flags
    lv_obj_clear_flag(update_dropdown, LV_OBJ_FLAG_GESTURE_BUBBLE); /// Flags
    lv_obj_set_style_text_font(update_dropdown, &ui_font_Inter_Bold_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(update_dropdown, &lv_font_montserrat_14, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lv_dropdown_get_list(update_dropdown), &ui_font_Inter_14,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lv_dropdown_get_list(update_dropdown), &ui_font_Inter_14,
                               LV_PART_SELECTED | LV_STATE_DEFAULT);

    // set change callback to change_update_selection
    lv_obj_add_event_cb(update_dropdown, change_update_selection, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_clear_flag(ui_UpdateBodyLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui_UpdatePrimaryActionButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_flex_align(ui_UpdateBody, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    break;
  case UPDATE_STEP_NO_UPDATE:
    lv_label_set_text(ui_UpdateHeaderLabel, "No Update");
    lv_label_set_text(ui_UpdateBodyLabel, "No updates available.");
    lv_label_set_text(ui_UpdatePrimaryActionButtonLabel, "Exit");
    lv_obj_clear_flag(ui_UpdateBodyLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui_UpdatePrimaryActionButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_flex_align(ui_UpdateBody, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    break;
  case UPDATE_STEP_DOWNLOADING:
    lv_label_set_text(ui_UpdateHeaderLabel, "Download");
    lv_label_set_text(ui_UpdateBodyLabel, "Downloading update...");
    lv_obj_add_flag(ui_UpdatePrimaryActionButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui_UpdateBodyLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui_UpdatePrimaryActionButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_flex_align(ui_UpdateBody, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    break;
  case UPDATE_STEP_VALIDATE:
    lv_label_set_text(ui_UpdateHeaderLabel, "Validation");
    lv_label_set_text(ui_UpdateBodyLabel, "Validating downloaded update...");
    lv_label_set_text(ui_UpdatePrimaryActionButtonLabel, "Stop");
    lv_obj_clear_flag(ui_UpdateBodyLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui_UpdatePrimaryActionButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_flex_align(ui_UpdateBody, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    break;
  case UPDATE_STEP_INSTALLING:
    lv_label_set_text(ui_UpdateHeaderLabel, "Installing");
    lv_label_set_text(ui_UpdateBodyLabel, "Installing update...");
    lv_label_set_text(ui_UpdatePrimaryActionButtonLabel, "Stop");
    lv_obj_clear_flag(ui_UpdateBodyLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_UpdatePrimaryActionButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_flex_align(ui_UpdateBody, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    break;
  case UPDATE_STEP_COMPLETE:
    lv_label_set_text(ui_UpdateHeaderLabel, "Complete");
    lv_label_set_text(ui_UpdateBodyLabel, "Update complete! Restarting...");
    lv_label_set_text(ui_UpdatePrimaryActionButtonLabel, "Reboot");
    lv_obj_clear_flag(ui_UpdateBodyLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui_UpdatePrimaryActionButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_flex_align(ui_UpdateBody, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    break;
  case UPDATE_STEP_ERROR:
    lv_label_set_text(ui_UpdateHeaderLabel, "Error");
    lv_label_set_text(ui_UpdateBodyLabel, "An error occurred during the update process.");
    lv_label_set_text(ui_UpdatePrimaryActionButtonLabel, "Retry");
    lv_obj_clear_flag(ui_UpdateBodyLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui_UpdatePrimaryActionButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_flex_align(ui_UpdateBody, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    break;
  case UPDATE_STEP_NO_WIFI:
    lv_label_set_text(ui_UpdateHeaderLabel, "No WiFi");
    lv_label_set_text(ui_UpdateBodyLabel,
                      "No WiFi credentials found. Please configure them at https://pubmote.techfoundry.nz");
    lv_label_set_text(ui_UpdatePrimaryActionButtonLabel, "Exit");
    lv_obj_clear_flag(ui_UpdateBodyLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui_UpdatePrimaryActionButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_flex_align(ui_UpdateBody, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    break;
  default:
    lv_label_set_text(ui_UpdateBodyLabel, "Unknown update step.");
    lv_obj_clear_flag(ui_UpdateBodyLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui_UpdatePrimaryActionButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_flex_align(ui_UpdateBody, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    break;
  }
  resize_footer_buttons(ui_UpdateFooter); // Resize footer buttons
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

void update_task(void *pvParameters) {
  connection_update_state(CONNECTION_STATE_DISCONNECTED);
  if (espnow_is_initialized()) {
    espnow_deinit();
  }
  if (!wifi_is_initialized()) {
    wifi_init();
  }

  UpdateStep last_step = current_update_step;
  char *wifi_ssid = get_wifi_ssid();
  char *wifi_password = get_wifi_password();
  update_status_label();
  while (is_update_screen_active()) {
    bool step_has_changed = (current_update_step != last_step);
    if (step_has_changed) {
      last_step = current_update_step;
      ESP_LOGI(TAG, "Update step changed: %d", current_update_step);
      if (LVGL_lock(0)) {
        update_status_label();
        LVGL_unlock();
      }
    }

    switch (current_update_step) {
    case UPDATE_STEP_START:
      if (wifi_ssid == NULL || strlen(wifi_ssid) == 0) {
        current_update_step = UPDATE_STEP_NO_WIFI;
      }

      break;
    case UPDATE_STEP_CONNECTING:
      esp_err_t wifi_err = ESP_OK;
      // check if already connected
      if (wifi_get_connection_state() != WIFI_STATE_CONNECTED) {
        wifi_err = wifi_connect_to_network(wifi_ssid, wifi_password);
      }
      if (wifi_get_connection_state() != WIFI_STATE_CONNECTED || wifi_err != ESP_OK) {
        current_update_step = UPDATE_STEP_ERROR;
      }
      else {
        current_update_step = UPDATE_STEP_CHECKING_UPDATE;
      }
      break;
    case UPDATE_STEP_CHECKING_UPDATE:
      // Check for updates
      // Fetch releases from GitHub
      // If update available, current_update_step = UPDATE_STEP_UPDATE_AVAILABLE
      // Else current_update_step = UPDATE_STEP_NO_UPDATE
      const char *asset_name = HW_TYPE;
      github_asset_urls_t result = {};
      esp_err_t err = fetch_all_asset_urls(asset_name, &result);

      if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error fetching asset URLs: %s", esp_err_to_name(err));
        current_update_step = UPDATE_STEP_ERROR;
        break;
      }

#if FORCE_UPDATE
      bool has_stable_update = result.stable_found;
      bool has_prerelease_update = result.prerelease_found;
#else
      firmware_version_t stable_version = parse_version_string(result.stable_tag);
      firmware_version_t prerelease_version = parse_version_string(result.prerelease_tag);
      firmware_version_t current_version = {.major = VERSION_MAJOR, .minor = VERSION_MINOR, .patch = VERSION_PATCH};
      bool has_stable_update = result.stable_found && is_version_greater(&stable_version, &current_version);
      bool has_prerelease_update = result.prerelease_found && is_version_greater(&prerelease_version, &current_version);
#endif

      if (has_stable_update) {
        ReleaseInfo stable_info = {
            .type = UPDATE_TYPE_STABLE,
        };
        strncpy(stable_info.tag_name, result.stable_tag, sizeof(stable_info.tag_name) - 1);
        snprintf(stable_info.name, sizeof(stable_info.name), "%s", result.stable_tag);
        strncpy(stable_info.download_url, result.stable_url, sizeof(stable_info.download_url) - 1);
        available_updates[available_update_count++] = stable_info;
      }

      if (has_prerelease_update) {
        ReleaseInfo prerelease_info = {
            .type = UPDATE_TYPE_PRERELEASE,
        };
        strncpy(prerelease_info.tag_name, result.prerelease_tag, sizeof(prerelease_info.tag_name) - 1);
        snprintf(prerelease_info.name, sizeof(prerelease_info.name), "%s (Prerelease)", result.prerelease_tag);
        strncpy(prerelease_info.download_url, result.prerelease_url, sizeof(prerelease_info.download_url) - 1);
        available_updates[available_update_count++] = prerelease_info;
      }

      ESP_LOGI(TAG, "Available updates count: %d", available_update_count);
      for (int i = 0; i < available_update_count; i++) {
        ESP_LOGI(TAG, "Update %d: Type=%d, Tag=%s, URL=%s", i, available_updates[i].type, available_updates[i].tag_name,
                 available_updates[i].download_url);
      }

      if (has_stable_update || has_prerelease_update) {
        current_update_step = UPDATE_STEP_UPDATE_AVAILABLE;
      }
      else {
        current_update_step = UPDATE_STEP_NO_UPDATE;
      }

      break;
    case UPDATE_STEP_UPDATE_AVAILABLE:
      // Do nothing
      break;
    case UPDATE_STEP_NO_UPDATE:
      // Do nothing
      break;
    case UPDATE_STEP_DOWNLOADING:
      // Start downloading update
      esp_http_client_config_t config = {
          .url = available_updates[selected_update_type].download_url,
          .skip_cert_common_name_check = true,
          .use_global_ca_store = false,
          .cert_pem = NULL,
          .client_cert_pem = NULL,
          .client_key_pem = NULL,
      };
      esp_https_ota_config_t ota_config = {
          .http_config = &config,
      };
      esp_err_t ret = esp_https_ota(&ota_config);
      if (ret == ESP_OK) {
        esp_restart();
      }
      else {
        ESP_LOGE(TAG, "OTA update failed: %s", esp_err_to_name(ret));
        current_update_step = UPDATE_STEP_ERROR;
      }
      break;
    case UPDATE_STEP_VALIDATE:
      // Validate downloaded update
      break;
    case UPDATE_STEP_INSTALLING:
      // Start installing update
      break;
    case UPDATE_STEP_COMPLETE:
      break;
    case UPDATE_STEP_NO_WIFI:
      // Handle no WiFi case
      break;
    case UPDATE_STEP_ERROR:
      // Handle error case
      break;
    default:
      break;
    }
    vTaskDelay(pdMS_TO_TICKS(LV_DISP_DEF_REFR_PERIOD));
  }

  // Cleanup
  if (wifi_is_initialized()) {
    wifi_uninit();
  }
  if (!espnow_is_initialized()) {
    espnow_init();
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
  xTaskCreate(update_task, "update_task", 8192, NULL, 2, NULL);
}

void update_screen_unload_start(lv_event_t *e) {
  ESP_LOGI(TAG, "Update screen unload start");
}

void update_primary_button_press(lv_event_t *e) {
  switch (current_update_step) {
  case UPDATE_STEP_START:
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