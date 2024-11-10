
#ifndef __STATS_H
#define __STATS_H
#include <stdio.h>

typedef enum {
  SPEED_UNIT_KMH,
  SPEED_UNIT_MPH
} SpeedUnit;

typedef enum {
  TEMP_UNIT_CELSIUS,
  TEMP_UNIT_FAHRENHEIT
} TempUnit;

typedef enum {
  CONNECTION_STATE_DISCONNECTED,
  CONNECTION_STATE_RECONNECTING,
  CONNECTION_STATE_CONNECTED
} ConnectionState;

typedef enum {
  SWITCH_STATE_OFF,
  SWITCH_STATE_LEFT,
  SWITCH_STATE_RIGHT,
  SWITCH_STATE_BOTH
} SwitchState;

typedef struct {
  float speed;
  float dutyCycle;
  SpeedUnit speedUnit;
  TempUnit tempUnit;
  float batteryVoltage;
  float batteryPercentage;
  uint8_t signalStrength;
  SwitchState switchState;
  ConnectionState connectionState;
} RemoteStats;

extern RemoteStats remoteStats;

void update_stats_display();
void init_stats();

#endif