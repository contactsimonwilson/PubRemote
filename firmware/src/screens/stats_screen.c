#include "screens/stats_screen.h"
#include "esp_log.h"
#include "remote/display.h"
#include "utilities/screen_utils.h"
#include <colors.h>
#include <core/lv_event.h>
#include <remote/connection.h>
#include <remote/settings.h>
#include <remote/stats.h>
#include <utilities/conversion_utils.h>

static const char *TAG = "PUBREMOTE-STATS_SCREEN";

StatsScreenDisplayOptions stat_display_options = {
    .primary_stat = STAT_DISPLAY_SPEED,
    .secondary_stat = STAT_DISPLAY_DUTY,
    .battery_display = BATTERY_DISPLAY_PERCENT,
};

static void change_stat_display(int direction) {
  if (direction > 0) {
    stat_display_options.primary_stat = (stat_display_options.primary_stat + 1) % 4;
  }
  else {
    stat_display_options.primary_stat = (stat_display_options.primary_stat + 3) % 4;
  }
}

bool is_stats_screen_active() {
  lv_obj_t *active_screen = lv_scr_act();
  return active_screen == ui_StatsScreen;
}

static uint8_t max_speed = 0;

static void update_speed_dial_display() {
  static float last_value = 0;

  if (last_value == remoteStats.speed) {
    return;
  }

  if (!max_speed) {
    /// get range from arc in case it was not set
    max_speed = lv_arc_get_max_value(ui_SpeedDial);
  }

  if (remoteStats.speed > max_speed) {
    max_speed = (uint8_t)remoteStats.speed;
    // Set range based on max speed

    lv_arc_set_range(ui_SpeedDial, 0, max_speed);
    lv_bar_set_range(ui_SpeedBar, 0, max_speed);
  }

  lv_arc_set_value(ui_SpeedDial, remoteStats.speed);
  lv_bar_set_value(ui_SpeedBar, remoteStats.speed, LV_ANIM_OFF);
  last_value = remoteStats.speed;
}

static void update_utilization_dial_display() {
  static uint8_t last_value = 0;

  if (last_value == remoteStats.dutyCycle) {
    return;
  }

  lv_arc_set_value(ui_UtilizationDial, remoteStats.dutyCycle);
  lv_bar_set_value(ui_UtilizationBar, remoteStats.dutyCycle, LV_ANIM_OFF);

  // set arc color
  lv_color_t color = lv_color_hex(COLOR_STRUCTURE);

  if (remoteStats.dutyCycle > 90) {
    color = lv_color_hex(COLOR_DANGER);
  }
  else if (remoteStats.dutyCycle > 80) {
    color = lv_color_hex(COLOR_ALERT);
  }
  else if (remoteStats.dutyCycle > 70) {
    color = lv_color_hex(COLOR_WARNING);
  }

  lv_obj_set_style_arc_color(ui_UtilizationDial, color, LV_PART_INDICATOR | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(ui_UtilizationBar, lv_color_hex(0x282828), LV_PART_INDICATOR | LV_STATE_DEFAULT);

  last_value = remoteStats.dutyCycle;
}

static void update_primary_stat_display() {
  static float last_value = 0;

  if (last_value == remoteStats.speed) {
    return;
  }

  float converted_val = remoteStats.speed;

  if (device_settings.distance_units == DISTANCE_UNITS_IMPERIAL) {
    converted_val = convert_kph_to_mph(remoteStats.speed);
  }

  char *formattedString;

  if (converted_val >= 10) {
    asprintf(&formattedString, "%.0f", converted_val);
  }
  else {
    asprintf(&formattedString, "%.1f", converted_val);
  }

  lv_label_set_text(ui_PrimaryStat, formattedString);
  free(formattedString);
  last_value = remoteStats.speed;
}

static void update_secondary_stat_display() {
  static ConnectionState last_value = CONNECTION_STATE_DISCONNECTED;

  if (last_value == connection_state) {
    return;
  }

  switch (connection_state) {
  case CONNECTION_STATE_DISCONNECTED:
    lv_label_set_text(ui_SecondaryStat, "Disconnected");
    break;
  case CONNECTION_STATE_RECONNECTING:
    lv_label_set_text(ui_SecondaryStat, "Reconnecting");
    break;
  case CONNECTION_STATE_CONNECTING:
    lv_label_set_text(ui_SecondaryStat, "Connecting");
    break;
  case CONNECTION_STATE_CONNECTED:
    lv_label_set_text(ui_SecondaryStat, "Connected"); // TODO - apply based on secondary stat display option
    break;
  default:
    break;
  }

  last_value = connection_state;
}

static void update_footpad_display() {
  static SwitchState last_value = SWITCH_STATE_OFF;

  if (last_value == remoteStats.switchState) {
    return;
  }

  switch (remoteStats.switchState) {
  case SWITCH_STATE_OFF:
    lv_arc_set_value(ui_LeftSensor, 0);
    lv_arc_set_value(ui_RightSensor, 0);
    break;
  case SWITCH_STATE_LEFT:
    lv_arc_set_value(ui_LeftSensor, 1);
    lv_arc_set_value(ui_RightSensor, 0);
    break;
  case SWITCH_STATE_RIGHT:
    lv_arc_set_value(ui_LeftSensor, 0);
    lv_arc_set_value(ui_RightSensor, 1);
    break;
  case SWITCH_STATE_BOTH:
    lv_arc_set_value(ui_LeftSensor, 1);
    lv_arc_set_value(ui_RightSensor, 1);
    break;
  default:
    break;
  }

  last_value = remoteStats.switchState;
}

static void update_board_battery_display() {
  static uint8_t last_board_battery_value = 0;

  // Reset display to show 0% battery if disconnected
  if (connection_state == CONNECTION_STATE_DISCONNECTED) {
    remoteStats.batteryPercentage = 0.0;
  }

  if (last_board_battery_value == remoteStats.batteryPercentage) {
    return;
  }

  char *formattedString;

  asprintf(&formattedString, "%d%%", remoteStats.batteryPercentage);
  lv_label_set_text(ui_BoardBatteryDisplay, formattedString);

  free(formattedString);

  last_board_battery_value = remoteStats.batteryPercentage;
}

static void update_remote_battery_display() {
  static uint8_t last_remote_battery_value = 0;

  if (last_remote_battery_value == remoteStats.remoteBatteryPercentage) {
    return;
  }

  // Array of LVGL objects representing the battery levels
  lv_obj_t *battery_elements[] = {ui_BatteryFillEmpty, ui_BatteryFill25, ui_BatteryFill50, ui_BatteryFill75,
                                  ui_BatteryFillFull};

  // Define the ranges for each element
  int battery_ranges[][2] = {
      {0, 19},  // ui_BatteryFillEmpty
      {20, 34}, // ui_BatteryFill25
      {35, 49}, // ui_BatteryFill50
      {50, 74}, // ui_BatteryFill75
      {75, 100} // ui_BatteryFillFull
  };

  // Determine the active element based on the percentage
  for (int i = 0; i < 5; i++) {
    if (remoteStats.remoteBatteryPercentage >= battery_ranges[i][0] &&
        remoteStats.remoteBatteryPercentage <= battery_ranges[i][1] &&
        lv_obj_has_flag(battery_elements[i], LV_OBJ_FLAG_HIDDEN)) {
      lv_obj_clear_flag(battery_elements[i], LV_OBJ_FLAG_HIDDEN);
    }
    else {
      lv_obj_add_flag(battery_elements[i], LV_OBJ_FLAG_HIDDEN);
    }
  }

  last_remote_battery_value = remoteStats.remoteBatteryPercentage;
}

void update_stats_screen_display() {
  if (LVGL_lock(-1)) {
    if (device_settings.distance_units == DISTANCE_UNITS_METRIC) {
      lv_label_set_text(ui_PrimaryStatUnit, "kph");
    }
    else {
      lv_label_set_text(ui_PrimaryStatUnit, "mph");
    }

    update_speed_dial_display();
    update_utilization_dial_display();
    update_primary_stat_display();
    update_secondary_stat_display();
    update_footpad_display();
    update_board_battery_display();
    update_remote_battery_display();
    LVGL_unlock();
  }
}

// Event handlers
void stats_screen_load_start(lv_event_t *e) {
  ESP_LOGI(TAG, "Stats screen load start");
  // Permanent screen - don't apply scale

  if (LVGL_lock(-1)) {
#if UI_SHAPE
    lv_obj_add_flag(ui_SpeedBar, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_UtilizationBar, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui_SpeedDial, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui_UtilizationDial, LV_OBJ_FLAG_HIDDEN);
#else
    lv_obj_add_flag(ui_SpeedDial, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui_UtilizationDial, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui_SpeedBar, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui_UtilizationBar, LV_OBJ_FLAG_HIDDEN);
#endif
    LVGL_unlock();
  }
}

void stats_screen_loaded(lv_event_t *e) {
  ESP_LOGI(TAG, "Stats screen loaded");
  update_stats_screen_display();
}

void stats_screen_unloaded(lv_event_t *e) {
  ESP_LOGI(TAG, "Stats screen unloaded");
}

void stat_long_press(lv_event_t *e) {
  change_stat_display(1);
}

void stat_swipe_left(lv_event_t *e) {
  change_stat_display(1);
}

void stat_swipe_right(lv_event_t *e) {
  change_stat_display(-1);
}

void stats_footer_long_press(lv_event_t *e) {
  // Your code here
}
