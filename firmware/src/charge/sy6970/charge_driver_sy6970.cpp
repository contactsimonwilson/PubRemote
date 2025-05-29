#include "charge_driver_sy6970.hpp"
#include <stdio.h>
#include <cstring>
#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c.h"
#include "XPowersLib.h"

static const char *TAG = "PUBREMOTE-CHARGE_DRIVER_SY6970";

// SY6970 I2C address (0x6A is the typical address)
#define SY6970_ADDR                 0x6A
#define SY6970_I2C_NUM             I2C_NUM_1

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

// /**
//  * @brief Get power information from SY6970
//  */
// void get_power_info(void)
// {
//     ESP_LOGI(TAG, "SY6970 charge enabled: %s", sy6970.isEnableCharge() ? "YES" : "NO");
//     ESP_LOGI(TAG, "Battery Voltage: %u mv", sy6970.getBattVoltage());
//     ESP_LOGI(TAG, "Input Voltage: %u mv", sy6970.getVbusVoltage());
//     ESP_LOGI(TAG, "System Voltage: %u mv", sy6970.getSystemVoltage());
//     ESP_LOGI(TAG, "Charge Status: %s", sy6970.getChargeStatusString());
//     ESP_LOGI(TAG, "Charge Current: %u mA", sy6970.getChargeCurrent());
//   }

/**
 * @brief Set power parameters for SY6970
 */
esp_err_t set_power_parameters(void)
{
    esp_err_t ret = ESP_OK;
    
    // Set charge current (e.g., 1000mA)
    if (!sy6970.setChargerConstantCurr(640)) {
        ESP_LOGE(TAG, "Failed to set charge current");
        ret = ESP_FAIL;
    } else {
        ESP_LOGI(TAG, "Charge current set");
    }
    
    // Set input current limit (e.g., 500mA)
    if (!sy6970.setInputCurrentLimit(750)) {
        ESP_LOGE(TAG, "Failed to set input current limit");
        ret = ESP_FAIL;
    } else {
        ESP_LOGI(TAG, "Input current limit set");
    }

    sy6970.setPrechargeCurr(320);
    ESP_LOGI(TAG, "Precharge current set");

    sy6970.setBoostVoltage(5000); // Set boost voltage to 5.0V
    ESP_LOGI(TAG, "Boost voltage set to 5.0V");
    
    // Set charge voltage (e.g., 4.2V)
    if (!sy6970.setChargeTargetVoltage(4208)) {
        ESP_LOGE(TAG, "Failed to set charge voltage");
        ret = ESP_FAIL;
    } else {
        ESP_LOGI(TAG, "Charge voltage set");
    }

    // sy6970.enterHizMode();
    
  
    
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

extern "C" uint16_t sy6970_get_battery_voltage()
{
    // ESP_LOGI(TAG, "BATT: %u mV", sy6970.getBattVoltage());
    // ESP_LOGI(TAG, "VBUS: %u mV", sy6970.getVbusVoltage());
    // ESP_LOGI(TAG, "SYS: %u mV", sy6970.getSystemVoltage());
    // ESP_LOGI(TAG, "Charge Status: %s", sy6970.getChargeStatusString());
    // ESP_LOGI(TAG, "Charge Current: %u mA", sy6970.getChargeCurrent());

    return sy6970.getBattVoltage();
}