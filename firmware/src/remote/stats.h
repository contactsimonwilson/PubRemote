
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
  TRIP_DISTANCE_UNIT_KM,
  TRIP_DISTANCE_UNIT_MI
} TripDistanceUnit;

typedef enum {
  SWITCH_STATE_OFF,
  SWITCH_STATE_LEFT,
  SWITCH_STATE_RIGHT,
  SWITCH_STATE_BOTH
} SwitchState;

typedef struct {
  int64_t lastUpdated;
  // Stats
  // kph
  float speed;
  // kph
  float maxSpeed;
  // 0 to 100
  uint8_t dutyCycle;
  // Unit of speed measure
  SpeedUnit speedUnit;
  // Unit of temperature measure
  TempUnit tempUnit;
  // Board battery voltage
  float batteryVoltage;
  // 0 to 100
  uint8_t batteryPercentage;
  // Remote battery voltage
  float remoteBatteryVoltage;
  // 0 to 100
  uint8_t remoteBatteryPercentage;
  float tripDistance;
  float motorTemp;
  float controllerTemp;
  TripDistanceUnit tripDistanceUnit;
  // RSSI
  uint8_t signalStrength;
  // Footpad switch state
  SwitchState switchState;
} RemoteStats;

extern RemoteStats remoteStats;

void update_stats_display();
void init_stats();

#endif