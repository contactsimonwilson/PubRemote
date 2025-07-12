#include "charge_driver.h"
#include "config.h"
#include <esp_log.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static const char *TAG = "PUBREMOTE-CHARGE-DRIVER";

#if PMU_SY6970
  #include "sy6970/charge_driver_sy6970.hpp"
#else
  #include "adc/charge_driver_adc.h"
#endif

esp_err_t charge_driver_init() {
  ESP_LOGI(TAG, "Init charge driver");
#if PMU_SY6970
  return sy6970_charge_driver_init();
#else
  return adc_charge_driver_init();
#endif
}

RemotePowerState get_power_state() {
  ESP_LOGD(TAG, "Getting battery voltage");
#if PMU_SY6970
  return sy6970_get_power_state();
#else
  return adc_get_power_state();
#endif
}

char *charge_state_to_string(RemoteChargeState state) {
  switch (state) {
  case CHARGE_STATE_NOT_CHARGING:
    return "Discharging";
  case CHARGE_STATE_CHARGING:
    return "Charging";
  case CHARGE_STATE_DONE:
    return "Complete";
  case CHARGE_STATE_UNKNOWN:
    return "Unknown";
  default:
    return "Invalid";
  }
}

static const VoltagePoint dischargeCurve[] = {
    {MAX_BATTERY_VOLTAGE, 100.0f}, // Fully charged
    {4050, 90.0f},                 // High
    {3900, 80.0f},                 // Good
    {3800, 70.0f},                 // Above nominal
    {3750, 60.0f},                 // Near nominal
    {3700, 50.0f},                 // Nominal voltage point
    {3650, 40.0f},                 // Below nominal
    {3600, 30.0f},                 // Getting low
    {3500, 20.0f},                 // Low
    {3400, 10.0f},                 // Very low
    {MIN_BATTERY_VOLTAGE, 0.0f}    // Empty/cutoff
};

#define CURVE_SIZE (sizeof(dischargeCurve) / sizeof(VoltagePoint))

float battery_mv_to_percent(uint16_t voltage_mv) {
  // Handle edge cases
  if (voltage_mv >= dischargeCurve[0].voltage_mv) {
    return dischargeCurve[0].percentage;
  }
  if (voltage_mv <= dischargeCurve[CURVE_SIZE - 1].voltage_mv) {
    return dischargeCurve[CURVE_SIZE - 1].percentage;
  }

  // Find the two points to interpolate between
  for (int i = 0; i < (int)CURVE_SIZE - 1; i++) {
    if (voltage_mv <= dischargeCurve[i].voltage_mv && voltage_mv >= dischargeCurve[i + 1].voltage_mv) {
      // Linear interpolation between two points
      uint16_t v1 = dischargeCurve[i].voltage_mv;
      uint16_t v2 = dischargeCurve[i + 1].voltage_mv;
      float p1 = dischargeCurve[i].percentage;
      float p2 = dischargeCurve[i + 1].percentage;

      float percentage = p1 + ((float)(voltage_mv - v1) / (float)(v2 - v1)) * (p2 - p1);
      return percentage;
    }
  }

  return 0.0f; // Fallback - should not reach here
}