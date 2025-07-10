#include "charge_driver_sy6970.hpp"
#include <stdio.h>
#include <cstring>
#include "esp_log.h"
#include "esp_err.h"
// Define the i2c type before including XPowersLib
#define CONFIG_XPOWERS_ESP_IDF_NEW_API
#include "XPowersLib.h"
#include <charge/charge_driver.h>
#include "config.h"
#include "remote/i2c.h"

static const char *TAG = "PUBREMOTE-CHARGE_DRIVER_SY6970";

// SY6970 I2C address (0x6A is the typical address)
#define SY6970_ADDR                 0x6A

// SY6970 instance
static PowersSY6970 sy6970;

static int sy6970_read_reg(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, uint8_t len)
{
    esp_err_t result = i2c_read_with_mutex(device_addr, reg_addr, data, len, 500);
    return (result == ESP_OK) ? 0 : -1;  // XPowersLib expects 0=success, -1=failure
}

static int sy6970_write_reg(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, uint8_t len)
{
    esp_err_t result = i2c_write_with_mutex(device_addr, reg_addr, data, len, 500);
    return (result == ESP_OK) ? 0 : -1;  // XPowersLib expects 0=success, -1=failure
}

/**
 * @brief Initialize the SY6970 power management chip
 */
static esp_err_t sy6970_init()
{
    #if defined(PMU_SDA) && defined(PMU_SCL)
    
    // Test I2C communication first
    uint8_t test_data;
    esp_err_t test_result = i2c_read_with_mutex(SY6970_ADDR, 0x0B, &test_data, 1, 500);
    if (test_result != ESP_OK) {
        ESP_LOGE(TAG, "I2C communication test failed: %s", esp_err_to_name(test_result));
        return test_result;
    }
    ESP_LOGI(TAG, "I2C communication test passed, read: 0x%02X", test_data);

    // Initialize the SY6970 with the XPowersLib
    bool result = sy6970.begin(SY6970_ADDR, sy6970_read_reg, sy6970_write_reg);    
    if (!result) {
        ESP_LOGE(TAG, "Failed to initialize SY6970");
        return ESP_FAIL;
    }
    
    // Call init() after successful begin()
    sy6970.init();
    
    ESP_LOGI(TAG, "SY6970 initialized successfully");
    return ESP_OK;
    #else
    ESP_LOGE(TAG, "PMU_SDA and PMU_SCL must be defined for SY6970 initialization");
    return ESP_ERR_INVALID_ARG;
    #endif
}

/**
 * @brief Set power parameters for SY6970
 */
static esp_err_t set_power_parameters()
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
    if (!sy6970.setChargeTargetVoltage(4224)) {
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
    
    switch (status) {
        case PowersSY6970::CHARGE_STATE_NO_CHARGE:  // No charge
            state.chargeState = CHARGE_STATE_NOT_CHARGING;
            break;
        case PowersSY6970::CHARGE_STATE_PRE_CHARGE:  // Pre-charge
            state.chargeState = CHARGE_STATE_CHARGING;
            break;
        case PowersSY6970::CHARGE_STATE_FAST_CHARGE:  // Pre-charge
            state.chargeState = CHARGE_STATE_CHARGING;
            break;
        case PowersSY6970::CHARGE_STATE_DONE:  // Charge done
            state.chargeState = CHARGE_STATE_DONE;
            break;
        default:
            state.chargeState = CHARGE_STATE_UNKNOWN;
            break;
    }

    ESP_LOGI(TAG, "BATT: %u mV", sy6970.getBattVoltage());
    ESP_LOGI(TAG, "VBUS: %u mV", sy6970.getVbusVoltage());
    ESP_LOGI(TAG, "SYS: %u mV", sy6970.getSystemVoltage());
    ESP_LOGI(TAG, "Charge Status: %s", sy6970.getChargeStatusString());
    ESP_LOGI(TAG, "Charge Current: %u mA", sy6970.getChargeCurrent());

    return state;
}