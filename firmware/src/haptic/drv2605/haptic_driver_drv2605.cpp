#include "haptic_driver_drv2605.hpp"
#include "esp_log.h"
#include "esp_err.h"
#include "drv2605.hpp"
#include "remote/i2c.h"

static const char *TAG = "PUBREMOTE-HAPTIC_DRIVER_DRV2605";

// DRV2605 I2C address (0x5A is the typical address)
#define SY6970_ADDR                 0x5A

// DRV2605 instance
static espp::Drv2605* drv2605;

bool drv2605_read_reg(uint8_t device_addr, uint8_t reg_addr, uint8_t* data, size_t len) {
    esp_err_t result = i2c_read_with_mutex(device_addr, reg_addr, data, len, 500);
    
    // Return true for success, false for any error
    return (result == ESP_OK);
}

static bool drv2605_write_reg(uint8_t reg_addr, const uint8_t *data, size_t len)
{
    esp_err_t result = i2c_write_with_mutex(SY6970_ADDR, reg_addr, (uint8_t*)data, len, 500);

    // Return true for success, false for any error
    return (result == ESP_OK);
}

static esp_err_t drv2605_init()
{
    #if defined(HAPTIC_SDA) && defined(HAPTIC_SCL)
        // Configure DRV2605
    espp::Drv2605::Config drv_config{
        .device_address = SY6970_ADDR,
        .write = drv2605_write_reg,
        .read_register = drv2605_read_reg,
        .motor_type = espp::Drv2605::MotorType::LRA, // or LRA
        .auto_init = true,
        .log_level = espp::Logger::Verbosity::INFO
    };

    // Create DRV2605 instance
    espp::Drv2605 drv(drv_config);
    drv2605 = &drv;
    #else
    ESP_LOGE(TAG, "HAPTIC_SDA and HAPTIC_SCL must be defined for DRV2605 initialization");
    return ESP_ERR_INVALID_ARG;
    #endif
    return ESP_OK;
}


void drv2605_haptic_play_vibration(HapticFeedbackPattern pattern) {
    std::error_code ec;
    // Play the specified vibration pattern using the DRV2605 haptic driver
    ESP_LOGI(TAG, "Playing vibration pattern: %d", pattern);

    drv2605->set_waveform(0, espp::Drv2605::Waveform::STRONG_CLICK, ec);
    drv2605->set_waveform(1, espp::Drv2605::Waveform::END, ec); // Always end with END
    drv2605->start(ec);

    ESP_LOGI(TAG, "Vibration pattern %d played successfully", pattern);
}

esp_err_t drv2605_haptic_driver_init() {
    // Initialize the DRV2605 haptic driver
    ESP_LOGI("DRV2605", "Initializing DRV2605 haptic driver");
    drv2605_init();
    ESP_LOGI("DRV2605", "DRV2605 haptic driver initialized successfully");
    return ESP_OK;
}