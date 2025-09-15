#include "screens/menu_screen.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "utilities/screen_utils.h"
#include <remote/connection.h>
#include <remote/display.h>
#include <remote/powermanagement.h>
#include <remote/settings.h>
#include <ui/ui.h>

static const char *TAG = "PUBREMOTE-MENU_SCREEN";

lv_obj_t *ui_MenuBackButton = NULL, *ui_MenuBackButtonLabel = NULL, *ui_MenuConnectButton = NULL,
         *ui_MenuConnectButtonLabel = NULL, *ui_MenuPocketModeButton = NULL, *ui_MenuPocketModeButtonLabel = NULL,
         *ui_MenuSettingsButton = NULL, *ui_MenuSettingsButtonLabel = NULL, *ui_MenuCalibrateButton = NULL,
         *ui_MenuCalibrateButtonLabel = NULL, *ui_MenuPairButton = NULL, *ui_MenuPairButtonLabel = NULL,
         *ui_MenuAboutButton = NULL, *ui_MenuAboutButtonLabel = NULL, *ui_MenuShutdownButton = NULL,
         *ui_MenuShutdownButtonLabel = NULL;

static bool confirm_reset = false;

static void set_reset_mode(bool mode) {
  confirm_reset = mode;

  if (confirm_reset) {
    lv_label_set_text(ui_MenuShutdownButtonLabel, "Factory reset?");
  }

  else {
    lv_label_set_text(ui_MenuShutdownButtonLabel, "Shutdown");
  }
}

bool is_menu_screen_active() {
  lv_obj_t *active_screen = lv_scr_act();
  return active_screen == ui_MenuScreen;
}

void add_main_menu_navigation_buttons(lv_obj_t *body_item) {
  // Define button entries and their associated actions/screens
  typedef struct {
    lv_obj_t **btn;
    lv_obj_t **label;
    const char *text;
    bool hidden;
    void (*short_press)(lv_event_t *e);
    void (*long_press)(lv_event_t *e);
    void (*down)(lv_event_t *e);
  } menu_button_t;

  menu_button_t button_entries[] = {
      {&ui_MenuBackButton, &ui_MenuBackButtonLabel, "Back", false, menu_back_press, NULL, NULL},
      {&ui_MenuConnectButton, &ui_MenuConnectButtonLabel, "Connect", true, menu_connect_press, NULL, NULL},
      {&ui_MenuPocketModeButton, &ui_MenuPocketModeButtonLabel, "Enable Pocket Mode", false, menu_pocket_mode_press,
       NULL, NULL},
      {&ui_MenuSettingsButton, &ui_MenuSettingsButtonLabel, "Settings", false, menu_settings_press, NULL, NULL},
      {&ui_MenuCalibrateButton, &ui_MenuCalibrateButtonLabel, "Calibration", false, menu_calibration_press, NULL, NULL},
      {&ui_MenuPairButton, &ui_MenuPairButtonLabel, "Pairing", false, menu_pair_press, NULL, NULL},
      {&ui_MenuAboutButton, &ui_MenuAboutButtonLabel, "About", false, menu_about_press, NULL, NULL},
      {&ui_MenuShutdownButton, &ui_MenuShutdownButtonLabel, "Shutdown", false, menu_shutdown_button_press,
       menu_shutdown_button_long_press, menu_shutdown_button_down}};

  int total_buttons = sizeof(button_entries) / sizeof(button_entries[0]);

  for (uint32_t i = 0; i < total_buttons; i++) {
    lv_obj_t *item = NULL;
    if (lv_obj_get_child_cnt(body_item) > i) {
      item = lv_obj_get_child(body_item, i);
      if (item == NULL) {
        item = lv_btn_create(body_item);
      }
    }
    else {
      item = lv_btn_create(body_item);
    }

    lv_obj_set_height(item, 42);
    lv_obj_set_width(item, lv_pct(100));
    lv_obj_set_align(item, LV_ALIGN_CENTER);
    lv_obj_add_flag(item, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);

    if (button_entries[i].hidden) {
      lv_obj_add_flag(item, LV_OBJ_FLAG_HIDDEN);
    }
    else {
      lv_obj_clear_flag(item, LV_OBJ_FLAG_HIDDEN);
    }

    if (button_entries[i].short_press) {
      lv_obj_add_event_cb(item, button_entries[i].short_press, LV_EVENT_CLICKED, NULL);
    }
    if (button_entries[i].long_press) {
      lv_obj_add_event_cb(item, button_entries[i].long_press, LV_EVENT_LONG_PRESSED, NULL);
    }
    if (button_entries[i].down) {
      lv_obj_add_event_cb(item, button_entries[i].down, LV_EVENT_PRESSED, NULL);
    }

    lv_obj_t *label = NULL;
    if (lv_obj_get_child_cnt(item) > 0) {
      label = lv_obj_get_child(item, 0);
    }
    else {
      label = lv_label_create(item);
    }
    lv_obj_set_width(label, LV_SIZE_CONTENT);
    lv_obj_set_height(label, LV_SIZE_CONTENT);
    lv_obj_set_align(label, LV_ALIGN_CENTER);
    lv_label_set_text(label, button_entries[i].text);

    // Associate the created objects with the global pointers via the struct
    *(button_entries[i].btn) = item;
    *(button_entries[i].label) = label;
  }
}

// Event handlers
void menu_screen_load_start(lv_event_t *e) {
  ESP_LOGI(TAG, "Menu screen load start");
  // Permanent screen - don't apply scale

  if (LVGL_lock(0)) {
    // Add menu buttons if not already present
    if (lv_obj_get_child_cnt(ui_MenuBody) == 0) {
      add_main_menu_navigation_buttons(ui_MenuBody);
      apply_ui_scale(ui_MenuBody);
    }

    create_navigation_group(ui_MenuBody);

    LVGL_unlock();
  }
}

void menu_screen_loaded(lv_event_t *e) {
  ESP_LOGI(TAG, "Menu screen loaded");

  set_reset_mode(false);

  if (LVGL_lock(0)) {
    if (pairing_state == PAIRING_STATE_PAIRED) {
      lv_obj_clear_flag(ui_MenuConnectButton, LV_OBJ_FLAG_HIDDEN);

      if (connection_state == CONNECTION_STATE_DISCONNECTED) {
        lv_label_set_text(ui_MenuConnectButtonLabel, "Connect");
      }
      else {
        lv_label_set_text(ui_MenuConnectButtonLabel, "Disconnect");
      }
    }
    else {
      lv_obj_add_flag(ui_MenuConnectButton, LV_OBJ_FLAG_HIDDEN);
    }

    // Update pocket mode button text
    if (is_pocket_mode_enabled()) {
      lv_label_set_text(ui_MenuPocketModeButtonLabel, "Disable Pocket Mode");
    }
    else {
      lv_label_set_text(ui_MenuPocketModeButtonLabel, "Enable Pocket Mode");
    }

    LVGL_unlock();
  }
}

void menu_screen_unload_start(lv_event_t *e) {
  ESP_LOGI(TAG, "Menu screen unload start");
}

void enter_deep_sleep(lv_event_t *e) {
  enter_sleep();
}

void menu_back_press(lv_event_t *e) {
  ESP_LOGI(TAG, "Back button pressed");

  if (LVGL_lock(0)) {
    _ui_screen_change(&ui_StatsScreen, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_StatsScreen_screen_init);
    LVGL_unlock();
  }
}

void menu_connect_press(lv_event_t *e) {
  ESP_LOGI(TAG, "Connect button pressed");

  if (connection_state == CONNECTION_STATE_DISCONNECTED) {
    connection_connect_to_default_peer();
  }
  else {
    connection_update_state(CONNECTION_STATE_DISCONNECTED);
  }

  if (LVGL_lock(0)) {
    _ui_screen_change(&ui_StatsScreen, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_StatsScreen_screen_init);
    LVGL_unlock();
  }
}

void menu_pocket_mode_press(lv_event_t *e) {
  ESP_LOGI(TAG, "Pocket mode button pressed");

  // Toggle pocket mode
  if (device_settings.pocket_mode == POCKET_MODE_DISABLED) {
    device_settings.pocket_mode = POCKET_MODE_ENABLED;
  }
  else {
    device_settings.pocket_mode = POCKET_MODE_DISABLED;
  }

  save_device_settings();

  if (LVGL_lock(0)) {
    _ui_screen_change(&ui_StatsScreen, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_StatsScreen_screen_init);
    LVGL_unlock();
  }
}

void menu_settings_press(lv_event_t *e) {
  ESP_LOGI(TAG, "Settings button pressed");

  if (LVGL_lock(0)) {
    _ui_screen_change(&ui_SettingsScreen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, &ui_SettingsScreen_screen_init);
    LVGL_unlock();
  }
}

void menu_calibration_press(lv_event_t *e) {
  ESP_LOGI(TAG, "Calibration button pressed");

  if (LVGL_lock(0)) {
    _ui_screen_change(&ui_CalibrationScreen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, &ui_CalibrationScreen_screen_init);
    LVGL_unlock();
  }
}

void menu_pair_press(lv_event_t *e) {
  ESP_LOGI(TAG, "Pairing button pressed");

  if (LVGL_lock(0)) {
    _ui_screen_change(&ui_PairingScreen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, &ui_PairingScreen_screen_init);
    LVGL_unlock();
  }
}

void menu_about_press(lv_event_t *e) {
  ESP_LOGI(TAG, "About button pressed");

  if (LVGL_lock(0)) {
    _ui_screen_change(&ui_AboutScreen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, &ui_AboutScreen_screen_init);
    LVGL_unlock();
  }
}

// Track press event so we can bind long and short press events without overlap
static bool is_press_handled = false;

void menu_shutdown_button_down(lv_event_t *e) {
  is_press_handled = false;
}

void menu_shutdown_button_press(lv_event_t *e) {
  if (is_press_handled) {
    ESP_LOGI(TAG, "Shutdown button press already handled");
    return;
  }

  ESP_LOGI(TAG, "Shutdown button press");
  if (!confirm_reset) {
    enter_sleep();
  }
  else {
    reset_all_settings();
    esp_restart();
  }
}

void menu_shutdown_button_long_press(lv_event_t *e) {
  if (is_press_handled) {
    ESP_LOGI(TAG, "Shutdown button long press already handled");
    return;
  }
  ESP_LOGI(TAG, "Shutdown button long press");
  set_reset_mode(!confirm_reset);
  is_press_handled = true;
}
