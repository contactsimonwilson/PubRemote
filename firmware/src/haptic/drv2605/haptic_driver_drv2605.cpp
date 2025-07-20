#include "haptic_driver_drv2605.hpp"
#include "esp_log.h"
#include "esp_err.h"
#include "drv2605.hpp"
#include "remote/i2c.h"
#include <driver/gpio.h>

static const char *TAG = "PUBREMOTE-HAPTIC_DRIVER_DRV2605";

// DRV2605 I2C address (0x5A is the typical address)
#define SY6970_ADDR                 0x5A
// LRA configuration parameters - STRONG BUT SAFE SETTINGS
#define LRA_RATED_VOLTAGE = 1.2  // 1.2V RMS
#define LRA_OVERDRIVE_VOLTAGE = 2.4  // 2x rated voltage (safe maximum)
#define LRA_FREQUENCY = 170  // 170Hz ±5Hz
#define LRA_IMPEDANCE = 9.0  // 9Ω ±2Ω

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
    std::error_code ec;

    #ifdef HAPTIC_EN
    // Enable pin
    gpio_reset_pin((gpio_num_t)HAPTIC_EN); // Initialize the pin
    gpio_set_direction((gpio_num_t)HAPTIC_EN, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)HAPTIC_EN, 1); // Turn on the LED power
    #endif

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
    drv2605->select_library(espp::Drv2605::Library::LRA, ec); // Select LRA library
    if (ec) {
        ESP_LOGE(TAG, "Failed to select LRA library: %s", ec.message().c_str());
        return ESP_FAIL;
    }
    // do the calibration for the LRA motor
    espp::Drv2605::LraCalibrationSettings lra_calibration_settings{};
    lra_calibration_settings.rated_voltage = 255;
    lra_calibration_settings.overdrive_clamp = 255;
    lra_calibration_settings.drive_time = espp::Drv2605::lra_freq_to_drive_time(200.0f);
    espp::Drv2605::CalibratedData calibrated_data;
    if (!drv2605->calibrate(lra_calibration_settings, calibrated_data, ec)) {
      ESP_LOGE(TAG, "calibration failed: %s", ec.message().c_str());
      return ESP_FAIL;
    }

    // drv2605->initialize();
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