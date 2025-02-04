#include <ui/ui.h>
#ifndef __STATS_SCREEN_H
  #define __STATS_SCREEN_H

typedef enum {
  STAT_DISPLAY_SPEED,
  STAT_DISPLAY_DUTY,
  STAT_DISPLAY_POWER,
  STAT_DISPLAY_TEMP,
} StatsDisplayOption;

typedef enum {
  BATTERY_DISPLAY_ALL,
  BATTERY_DISPLAY_PERCENT,
  BATTERY_DISPLAY_VOLTAGE,
} BoardBatteryDisplayOption;

typedef struct {
  StatsDisplayOption primary_stat;
  StatsDisplayOption secondary_stat;
  BoardBatteryDisplayOption battery_display;
} StatsScreenDisplayOptions;

typedef enum {
  SIGNAL_STRINGTH_NONE,
  SIGNAL_STRENGTH_POOR,
  SIGNAL_STRENGTH_FAIR,
  SIGNAL_STRENGTH_GOOD,
} SignalStrength;

extern StatsScreenDisplayOptions stat_display_options;
bool is_stats_screen_active();
void update_stats_screen_display();

void stats_screen_load_start(lv_event_t *e);
void stats_screen_loaded(lv_event_t *e);
void stats_screen_unloaded(lv_event_t *e);
void stat_long_press(lv_event_t *e);
void stat_swipe_left(lv_event_t *e);
void stat_swipe_right(lv_event_t *e);
void stats_footer_long_press(lv_event_t *e);

#endif