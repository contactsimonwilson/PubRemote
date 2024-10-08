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
} BatteryDisplayOption;

typedef struct {
  StatsDisplayOption primary_stat;
  StatsDisplayOption secondary_stat;
  BatteryDisplayOption battery_display;
} StatsScreenDisplayOptions;

extern StatsScreenDisplayOptions stat_display_options;
bool is_stats_screen_active();
void update_stats_screen_display();

void stat_long_press(lv_event_t *e);
void stat_swipe_left(lv_event_t *e);
void stat_swipe_right(lv_event_t *e);
void stats_footer_long_press(lv_event_t *e);

#endif