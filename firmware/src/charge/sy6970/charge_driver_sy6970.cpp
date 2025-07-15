#include "charge_driver_sy6970.hpp"
#include "esp_err.h"
#include "esp_log.h"
#include <cstring>
#include <stdio.h>
// Define the configuration for XPowersLib
#define CONFIG_XPOWERS_ESP_IDF_NEW_API
#define XPOWERS_CHIP_SY6970
#include "XPowersLib.h"
#include "config.h"
#include "remote/i2c.h"
#include <charge/charge_driver.h>
#include <driver/gpio.h>

static const char *TAG = "PUBREMOTE-CHARGE_DRIVER_SY6970";

// SY6970 I2C address (0x6A is the typical address)
#define SY6970_ADDR 0x6A

// SY6970 instance
static XPowersPPM PPM;

static int sy6970_read_reg(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, uint8_t len) {
  esp_err_t result = i2c_read_with_mutex(device_addr, reg_addr, data, len, 500);
  return (result == ESP_OK) ? 0 : -1; // XPowersLib expects 0=success, -1=failure
}

static int sy6970_write_reg(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, uint8_t len) {
  esp_err_t result = i2c_write_with_mutex(device_addr, reg_addr, data, len, 500);
  return (result == ESP_OK) ? 0 : -1; // XPowersLib expects 0=success, -1=failure
}

/**
 * @brief Initialize the SY6970 power management chip
 */
static esp_err_t sy6970_init() {
  // Initialize the SY6970 with the XPowersLib
  bool result = PPM.begin(SY6970_ADDR, sy6970_read_reg, sy6970_write_reg);
  if (!result) {
    ESP_LOGE(TAG, "Failed to initialize SY6970");
    return ESP_FAIL;
  }

  PPM.init();
  PPM.setSysPowerDownVoltage(3000); // Low voltage protection
  PPM.setInputCurrentLimit(1500);
  PPM.disableCurrentLimitPin();
  PPM.setChargeTargetVoltage(4224);
  PPM.setPrechargeCurr(128);
  PPM.setChargerConstantCurr(1024);
  PPM.enableMeasure(); // ADC must be enabled before reading voltages
  PPM.enableCharge();
  PPM.enableWatchdog(PowersSY6970::TIMER_OUT_40SEC);

  ESP_LOGI(TAG, "SY6970 initialized successfully");
  return ESP_OK;
}

extern "C" esp_err_t sy6970_charge_driver_init() {
  ESP_LOGI(TAG, "Initializing SY6970 charge driver");

  // Initialize SY6970
  esp_err_t ret = sy6970_init();
  if (ret != ESP_OK) {
    return ret;
  }

  ESP_LOGI(TAG, "SY6970 charge driver initialized successfully");
  return ESP_OK;
}

extern "C" RemotePowerState sy6970_get_power_state() {
  PPM.feedWatchdog();
  RemotePowerState state = {.voltage = 0, .chargeState = CHARGE_STATE_UNKNOWN, .current = 0, .isPowered = false, .isFault = false};
  state.voltage = PPM.getBattVoltage();
  state.current = PPM.getChargeCurrent();

  PowersSY6970::ChargeStatus status = PPM.chargeStatus();

  switch (status) {
  case PowersSY6970::CHARGE_STATE_NO_CHARGE: // No charge
    state.chargeState = CHARGE_STATE_NOT_CHARGING;
    break;
  case PowersSY6970::CHARGE_STATE_PRE_CHARGE: // Pre-charge
    state.chargeState = CHARGE_STATE_CHARGING;
    break;
  case PowersSY6970::CHARGE_STATE_FAST_CHARGE: // Pre-charge
    state.chargeState = CHARGE_STATE_CHARGING;
    break;
  case PowersSY6970::CHARGE_STATE_DONE: // Charge done
    state.chargeState = CHARGE_STATE_DONE;
    break;
  default:
    state.chargeState = CHARGE_STATE_UNKNOWN;
    break;
  }

  state.isPowered = PPM.isVbusIn();
  state.isFault = PPM.getFaultStatus() != 0;


  ESP_LOGD(TAG, "\nVBUS: %s %04dmV\nVBAT: %04dmV\nVSYS: %04dmV\nBus state: %s\nCharge state: %s\nCharge Current: %04dmA",
           PPM.isVbusIn() ? "Connected" : "Disconnect", PPM.getVbusVoltage(), PPM.getBattVoltage(),
           PPM.getSystemVoltage(), PPM.getBusStatusString(), PPM.getChargeStatusString(), PPM.getChargeCurrent());

  return state;
}

void sy6970_disable_watchdog() {
  PPM.disableWatchdog();
  ESP_LOGI(TAG, "SY6970 watchdog disabled");
}

void sy6970_enable_watchdog() {
  PPM.enableWatchdog(PowersSY6970::TIMER_OUT_40SEC);
  ESP_LOGI(TAG, "SY6970 watchdog enabled");
}