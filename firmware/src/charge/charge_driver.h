#ifndef __CHARGE_DRIVER_H
#define __CHARGE_DRIVER_H
#include <esp_err.h>
#include <esp_lcd_types.h>

typedef enum {
  CHARGE_STATE_NOT_CHARGING,
  CHARGE_STATE_CHARGING,
  CHARGE_STATE_DONE,
  CHARGE_STATE_UNKNOWN,
} RemoteChargeState;

typedef struct {
  uint16_t voltage;              // Battery voltage in mV
  RemoteChargeState chargeState; // Charging state
  uint16_t current;              // Charging current in mA
} RemotePowerState;

// Generic interface for display commands
esp_err_t charge_driver_init();
RemotePowerState get_power_state();

#endif