#include "stats.h"
#include <screens/stats_screen.h>

RemoteStats remoteStats;

void update_stats_display() {
  if (is_stats_screen_active()) {
    update_stats_screen_display();
  }
}

static void reset_stats() {
  remoteStats.speed = 0.0;
  remoteStats.dutyCycle = 0;
  remoteStats.speedUnit = SPEED_UNIT_KMH;
  remoteStats.tempUnit = TEMP_UNIT_CELSIUS;
  remoteStats.batteryVoltage = 0.0;
  remoteStats.batteryPercentage = 0.0;
  remoteStats.remoteBatteryVoltage = 0.0;
  remoteStats.remoteBatteryPercentage = 0;
  remoteStats.signalStrength = 0;
  remoteStats.switchState = SWITCH_STATE_OFF;
  remoteStats.connectionState = CONNECTION_STATE_DISCONNECTED;
}

void init_stats() {
  reset_stats();
  update_stats_display();
}
