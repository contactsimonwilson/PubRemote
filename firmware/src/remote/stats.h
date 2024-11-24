
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
  CONNECTION_STATE_CONNECTING,
  CONNECTION_STATE_CONNECTED,
  CONNECTION_STATE_RECONNECTING
} ConnectionState;

typedef enum {
  SWITCH_STATE_OFF,
  SWITCH_STATE_LEFT,
  SWITCH_STATE_RIGHT,
  SWITCH_STATE_BOTH
} SwitchState;

typedef struct {
  float speed;       // kph
  float maxSpeed;    // kph
  uint8_t dutyCycle; // 0 to 100
  SpeedUnit speedUnit;
  TempUnit tempUnit;
  float batteryVoltage;
  uint8_t batteryPercentage; // 0 to 100
  uint8_t signalStrength;    // RSSI
  SwitchState switchState;
  ConnectionState connectionState;
  float remoteBatteryVoltage;
  uint8_t remoteBatteryPercentage; // 0 to 100
} RemoteStats;

extern RemoteStats remoteStats;

void update_stats_display();
void init_stats();

#endif