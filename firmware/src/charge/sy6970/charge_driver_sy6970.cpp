#include "charge_driver_sy6970.hpp"
#include <stdio.h>
#include <cstring>
#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c.h"
#include "XPowersLib.h"
#include <charge/charge_driver.h>

static const char *TAG = "PUBREMOTE-CHARGE_DRIVER_SY6970";

// SY6970 I2C address (0x6A is the typical address)
#define SY6970_ADDR                 0x6A
#define SY6970_I2C_NUM             I2C_NUM_0

// SY6970 instance
static PowersSY6970 sy6970;

/**
 * @brief Initialize the SY6970 power management chip
 */
static esp_err_t sy6970_init(void)
{
    // Initialize the SY6970 with the XPowersLib
    bool result = sy6970.begin(SY6970_I2C_NUM, SY6970_ADDR, PMIC_SDA, PMIC_SCL);
    if (!result) {
        ESP_LOGE(TAG, "Failed to initialize SY6970");
        return ESP_FAIL;
    }
  
    
    ESP_LOGI(TAG, "SY6970 initialized successfully");
    return ESP_OK;
}

/**
 * @brief Set power parameters for SY6970
 */
static esp_err_t set_power_parameters(void)
{
    esp_err_t ret = ESP_OK;
    
    // // Set charge current (e.g., 1000mA)
    // if (!sy6970.setChargerConstantCurr(640)) {
    //     ESP_LOGE(TAG, "Failed to set charge current");
    //     ret = ESP_FAIL;
    // } else {
    //     ESP_LOGI(TAG, "Charge current set");
    // }
    
    // // Set input current limit (e.g., 500mA)
    // if (!sy6970.setInputCurrentLimit(750)) {
    //     ESP_LOGE(TAG, "Failed to set input current limit");
    //     ret = ESP_FAIL;
    // } else {
    //     ESP_LOGI(TAG, "Input current limit set");
    // }

    // sy6970.setPrechargeCurr(320);
    // ESP_LOGI(TAG, "Precharge current set");

    // sy6970.setBoostVoltage(5000); // Set boost voltage to 5.0V
    // ESP_LOGI(TAG, "Boost voltage set to 5.0V");
    
    // Set charge voltage (e.g., 4.2V)
    if (!sy6970.setChargeTargetVoltage(4208)) {
        ESP_LOGE(TAG, "Failed to set charge voltage");
        ret = ESP_FAIL;
    } else {
        ESP_LOGI(TAG, "Charge voltage set");
    }
    
    return ret;
}

extern "C" esp_err_t sy6970_charge_driver_init()
{
    ESP_LOGI(TAG, "Initializing SY6970 charge driver");

    // Initialize SY6970
    esp_err_t ret = sy6970_init();
    sy6970.init();
    if (ret != ESP_OK) {
        return ret;
    }

    // Set power parameters
    ret = set_power_parameters();
    if (ret != ESP_OK) {
        return ret;
    }

    ESP_LOGI(TAG, "SY6970 charge driver initialized successfully");
    return ESP_OK;
}

extern "C" RemotePowerState sy6970_get_power_state()
{
    RemotePowerState state = {.voltage = 0, .chargeState = CHARGE_STATE_UNKNOWN, .current = 0};
    state.voltage = sy6970.getBattVoltage();
    state.current = sy6970.getChargeCurrent();
    PowersSY6970::ChargeStatus status = sy6970.chargeStatus();
    if (status == PowersSY6970::CHARGE_STATE_NO_CHARGE) {
        state.chargeState = CHARGE_STATE_NOT_CHARGING;
    } else if (status == PowersSY6970::CHARGE_STATE_PRE_CHARGE) {
        state.chargeState = CHARGE_STATE_CHARGING;
    } else if (status == PowersSY6970::CHARGE_STATE_FAST_CHARGE) {
        state.chargeState = CHARGE_STATE_CHARGING;
    } else if (status == PowersSY6970::CHARGE_STATE_DONE) {
        state.chargeState = CHARGE_STATE_DONE;
    } else {
        state.chargeState = CHARGE_STATE_UNKNOWN;
    }


    ESP_LOGD(TAG, "BATT: %u mV", sy6970.getBattVoltage());
    ESP_LOGD(TAG, "VBUS: %u mV", sy6970.getVbusVoltage());
    ESP_LOGD(TAG, "SYS: %u mV", sy6970.getSystemVoltage());
    ESP_LOGD(TAG, "Charge Status: %s", sy6970.getChargeStatusString());
    ESP_LOGD(TAG, "Charge Current: %u mA", sy6970.getChargeCurrent());

    return state;
}