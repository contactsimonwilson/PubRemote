
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
  CONNECTION_STATE_CONNECTED
} ConnectionState;

typedef struct {
  float speed;
  SpeedUnit speedUnit;
  TempUnit tempUnit;
  float batteryVoltage;
  float batteryPercentage;
  uint8_t signalStrength;
  ConnectionState connectionState;
} RemoteDisplayState;

RemoteDisplayState displayState;

#endif