#include "screens/stats_screen.h"
#include "esp_log.h"
#include <core/lv_event.h>
#include <remote/stats.h>

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

static void update_speed_dial_display() {
  lv_arc_set_value(ui_SpeedDial, remoteStats.speed);
}

static void update_utilization_dial_display() {
  lv_arc_set_value(ui_UtilizationDial, remoteStats.dutyCycle);
}

static void update_primary_stat_display() {
  char *formattedString;
  asprintf(&formattedString, "%.1f", remoteStats.speed);
  lv_label_set_text(ui_PrimaryStat, formattedString);
}

static void update_secondary_stat_display() {
  switch (remoteStats.connectionState) {
  case CONNECTION_STATE_DISCONNECTED:
    lv_label_set_text(ui_SecondaryStat, "Disconnected");
    break;
  case CONNECTION_STATE_RECONNECTING:
    lv_label_set_text(ui_SecondaryStat, "Reconnecting");
    break;
  case CONNECTION_STATE_CONNECTED:
    lv_label_set_text(ui_SecondaryStat, "Connected"); // TODO - apply based on secondary stat display option
    break;
  default:
    break;
  }
}

static void update_footpad_display() {
  switch (remoteStats.switchState) {
  case 0:
    lv_arc_set_value(ui_LeftSensor, 0);
    lv_arc_set_value(ui_RightSensor, 0);
    break;
  case 1:
    lv_arc_set_value(ui_LeftSensor, 1);
    lv_arc_set_value(ui_RightSensor, 0);
    break;
  case 2:
    lv_arc_set_value(ui_LeftSensor, 0);
    lv_arc_set_value(ui_RightSensor, 1);
    break;
  case 3:
    lv_arc_set_value(ui_LeftSensor, 1);
    lv_arc_set_value(ui_RightSensor, 1);
    break;
  default:
    break;
  }
}

static void update_battery_display() {
}

void update_stats_screen_display() {
  update_speed_dial_display();
  update_utilization_dial_display();
  update_primary_stat_display();
  update_secondary_stat_display();
  update_footpad_display();
  update_battery_display();
}

// Event handlers
void stats_screen_loaded(lv_event_t *e) {
  ESP_LOGI(TAG, "Stats screen loaded");
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