
#ifndef __STATS_H
#define __STATS_H
#include "receiver.h"
#include <charge/charge_driver.h>
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
  SWITCH_STATE_OFF,
  SWITCH_STATE_LEFT,
  SWITCH_STATE_RIGHT,
  SWITCH_STATE_BOTH
} SwitchState;

typedef struct {
  int64_t lastUpdated;
  /* Stats */
  // Speed, stored in KPH
  float speed;
  // Max speed, stored in KPH
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
  uint16_t remoteBatteryVoltage;
  // 0 to 100
  uint8_t remoteBatteryPercentage;
  RemoteChargeState chargeState; // Current charge state
  uint16_t chargeCurrent;        // Charge current in mA
  // Board trip distance
  float tripDistance;
  // Board motor temperature
  float motorTemp;
  // Board controller temperature
  float controllerTemp;
  // RSSI
  int signalStrength;
  // Main board state
  BoardState state;
  // Footpad switch state
  SwitchState switchState;
} RemoteStats;

extern RemoteStats remoteStats;

void update_stats_display();
void init_stats();

#endif