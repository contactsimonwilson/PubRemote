#include "haptic_driver_drv2605.hpp"
#include "esp_log.h"
#include "esp_err.h"
#include "drv2605.hpp"
#include "remote/i2c.h"
#include <driver/gpio.h>
#include <memory>

static const char *TAG = "PUBREMOTE-HAPTIC_DRIVER_DRV2605";

// DRV2605 I2C address (0x5A is the typical address)
#define DRV2605_ADDR                0x5A

// LRA configuration parameters - STRONG BUT SAFE SETTINGS
#define LRA_RATED_VOLTAGE           1.2f    // 1.2V RMS
#define LRA_OVERDRIVE_VOLTAGE       2.4f    // 2x rated voltage (safe maximum)
#define LRA_FREQUENCY               170.0f  // 170Hz ±5Hz
#define LRA_IMPEDANCE               9.0f    // 9Ω ±2Ω

// Convert voltage to register value (assuming 0-255 range for 0-5.6V)
#define VOLTAGE_TO_REG(voltage)     (uint8_t)((voltage / 5.6f) * 255.0f)

// DRV2605 instance using smart pointer for better memory management
static std::unique_ptr<espp::Drv2605> drv2605;
static bool haptic_initialized = false;

static bool drv2605_write_reg(uint8_t reg_addr, const uint8_t *data, size_t len)
{
    esp_err_t result = i2c_write_with_mutex(DRV2605_ADDR, reg_addr, (uint8_t*)data, len, 500);
    return (result == ESP_OK);
}

bool drv2605_read_reg(uint8_t device_addr, uint8_t reg_addr, uint8_t* data, size_t len) {
    esp_err_t result = i2c_read_with_mutex(device_addr, reg_addr, data, len, 500);
    return (result == ESP_OK);
}

static esp_err_t drv2605_init()
{
    std::error_code ec;

    #ifdef HAPTIC_EN
    // Enable pin configuration
    gpio_reset_pin((gpio_num_t)HAPTIC_EN);
    gpio_set_direction((gpio_num_t)HAPTIC_EN, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)HAPTIC_EN, 1); // Enable haptic driver power
    ESP_LOGI(TAG, "Haptic enable pin configured and activated");
    
    // Small delay to allow power stabilization
    vTaskDelay(pdMS_TO_TICKS(50));
    #endif

    // Configure DRV2605
    espp::Drv2605::Config drv_config{
        .device_address = DRV2605_ADDR,
        .write = drv2605_write_reg,
        .read_register = drv2605_read_reg,
        .motor_type = espp::Drv2605::MotorType::LRA,
        .log_level = espp::Logger::Verbosity::DEBUG
    };

    // Create DRV2605 instance using smart pointer
    drv2605 = std::make_unique<espp::Drv2605>(drv_config);

    if (!drv2605) {
        ESP_LOGE(TAG, "Failed to create DRV2605 instance");
        return ESP_FAIL;
    }

        // Select LRA library
    bool lib_success = drv2605->select_library(espp::Drv2605::Library::LRA, ec);
    if (!lib_success || ec) {
        ESP_LOGE(TAG, "Failed to select LRA library: %s", ec.message().c_str());
        return ESP_FAIL;
    }

    // // Initialize the device
    // bool init_success = drv2605->initialize(ec);
    // if (!init_success || ec) {
    //     ESP_LOGE(TAG, "Failed to initialize DRV2605: %s", ec.message().c_str());
    //     return ESP_FAIL;
    // }
    ESP_LOGI(TAG, "LRA library selected successfully");

    // ===================================================================
    // LRA AUTOMATIC CALIBRATION PROCESS
    // ===================================================================
    // The DRV2605 needs to be calibrated for optimal LRA performance.
    // This process:
    // 1. Measures the LRA's back-EMF at resonance
    // 2. Determines optimal drive levels
    // 3. Sets up closed-loop control parameters
    // 4. Ensures maximum efficiency and consistent response
    // 
    // The motor WILL vibrate during this process - this is normal!
    // ===================================================================

    // Configure LRA calibration settings using our defined parameters
    ESP_LOGI(TAG, "=== LRA CALIBRATION STARTING ===");
    ESP_LOGI(TAG, "Motor specifications:");
    ESP_LOGI(TAG, "  - Rated voltage: %.1fV RMS", LRA_RATED_VOLTAGE);
    ESP_LOGI(TAG, "  - Overdrive voltage: %.1fV (%.1fx rated)", LRA_OVERDRIVE_VOLTAGE, LRA_OVERDRIVE_VOLTAGE/LRA_RATED_VOLTAGE);
    ESP_LOGI(TAG, "  - Resonant frequency: %.1fHz", LRA_FREQUENCY);
    ESP_LOGI(TAG, "  - Impedance: %.1fΩ", LRA_IMPEDANCE);

    espp::Drv2605::LraCalibrationSettings lra_calibration_settings{};
    lra_calibration_settings.rated_voltage = VOLTAGE_TO_REG(LRA_RATED_VOLTAGE);
    lra_calibration_settings.overdrive_clamp = VOLTAGE_TO_REG(LRA_OVERDRIVE_VOLTAGE);
    lra_calibration_settings.drive_time = espp::Drv2605::lra_freq_to_drive_time(LRA_FREQUENCY);
    
    ESP_LOGI(TAG, "Calibration register values:");
    ESP_LOGI(TAG, "  - Rated voltage reg: %d/255 (%.2fV)", lra_calibration_settings.rated_voltage, 
             (lra_calibration_settings.rated_voltage / 255.0f) * 5.6f);
    ESP_LOGI(TAG, "  - Overdrive clamp reg: %d/255 (%.2fV)", lra_calibration_settings.overdrive_clamp,
             (lra_calibration_settings.overdrive_clamp / 255.0f) * 5.6f);
    ESP_LOGI(TAG, "  - Drive time reg: %d (%.1fHz)", lra_calibration_settings.drive_time, LRA_FREQUENCY);

    ESP_LOGI(TAG, "Starting automatic LRA calibration...");
    ESP_LOGI(TAG, "⚠️  Motor will vibrate during calibration process");
    
    // Perform LRA motor calibration - this is the key step!
    espp::Drv2605::CalibratedData calibrated_data;
    uint32_t calibration_start = esp_log_timestamp();
    
    bool calibration_success = drv2605->calibrate(lra_calibration_settings, calibrated_data, ec);
    
    uint32_t calibration_duration = esp_log_timestamp() - calibration_start;
    
    if (!calibration_success) {
        ESP_LOGE(TAG, "❌ LRA calibration FAILED after %lums", calibration_duration);
        ESP_LOGE(TAG, "Error: %s", ec.message().c_str());
        ESP_LOGE(TAG, "Possible causes:");
        ESP_LOGE(TAG, "  - Motor not connected properly");
        ESP_LOGE(TAG, "  - Wrong motor specifications");
        ESP_LOGE(TAG, "  - I2C communication issues");
        ESP_LOGE(TAG, "  - Insufficient power supply");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "✅ LRA calibration SUCCESS! (took %lums)", calibration_duration);
    ESP_LOGI(TAG, "Motor is now optimally tuned for:");
    ESP_LOGI(TAG, "  - Maximum efficiency");
    ESP_LOGI(TAG, "  - Consistent haptic response");
    ESP_LOGI(TAG, "  - Proper frequency matching");
    ESP_LOGI(TAG, "=== LRA CALIBRATION COMPLETE ===");
    haptic_initialized = true;

    return ESP_OK;
}

void drv2605_haptic_play_vibration(HapticFeedbackPattern pattern) {
    if (!drv2605 || !haptic_initialized) {
        ESP_LOGE(TAG, "DRV2605 not initialized");
        return;
    }

    std::error_code ec;
    ESP_LOGI(TAG, "Playing vibration pattern: %d", (int)pattern);

    // Clear any existing waveforms
    drv2605->stop(ec);
    
    // Configure waveform based on pattern using available waveforms
    switch (pattern) {
        case HAPTIC_SINGLE_CLICK:
            drv2605->set_waveform(0, espp::Drv2605::Waveform::STRONG_CLICK, ec);
            drv2605->set_waveform(1, espp::Drv2605::Waveform::END, ec);
            break;
            
        case HAPTIC_DOUBLE_CLICK:
            drv2605->set_waveform(0, espp::Drv2605::Waveform::DOUBLE_CLICK, ec);
            drv2605->set_waveform(1, espp::Drv2605::Waveform::END, ec);
            break;
            
        case HAPTIC_TRIPLE_CLICK:
            drv2605->set_waveform(0, espp::Drv2605::Waveform::TRIPLE_CLICK, ec);
            drv2605->set_waveform(1, espp::Drv2605::Waveform::END, ec);
            break;
            
        case HAPTIC_SOFT_BUMP:
            drv2605->set_waveform(0, espp::Drv2605::Waveform::SOFT_BUMP, ec);
            drv2605->set_waveform(1, espp::Drv2605::Waveform::END, ec);
            break;
            
        case HAPTIC_STRONG_CLICK:
            drv2605->set_waveform(0, espp::Drv2605::Waveform::STRONG_CLICK, ec);
            drv2605->set_waveform(1, espp::Drv2605::Waveform::END, ec);
            break;
            
        case HAPTIC_SHARP_CLICK:
            drv2605->set_waveform(0, espp::Drv2605::Waveform::SHARP_CLICK, ec);
            drv2605->set_waveform(1, espp::Drv2605::Waveform::END, ec);
            break;
            
        case HAPTIC_SOFT_BUZZ:
            drv2605->set_waveform(0, espp::Drv2605::Waveform::SOFT_FUZZ, ec);
            drv2605->set_waveform(1, espp::Drv2605::Waveform::END, ec);
            break;
            
        case HAPTIC_STRONG_BUZZ:
            drv2605->set_waveform(0, espp::Drv2605::Waveform::STRONG_BUZZ, ec);
            drv2605->set_waveform(1, espp::Drv2605::Waveform::END, ec);
            break;
            
        case HAPTIC_ALERT_750MS:
            drv2605->set_waveform(0, espp::Drv2605::Waveform::ALERT_750MS, ec);
            drv2605->set_waveform(1, espp::Drv2605::Waveform::END, ec);
            break;
            
        case HAPTIC_ALERT_1000MS:
            drv2605->set_waveform(0, espp::Drv2605::Waveform::ALERT_1000MS, ec);
            drv2605->set_waveform(1, espp::Drv2605::Waveform::END, ec);
            break;
            
        default:
            ESP_LOGW(TAG, "Unknown vibration pattern: %d, using default", (int)pattern);
            drv2605->set_waveform(0, espp::Drv2605::Waveform::STRONG_CLICK, ec);
            drv2605->set_waveform(1, espp::Drv2605::Waveform::END, ec);
            break;
    }

    if (ec) {
        ESP_LOGE(TAG, "Failed to set waveform: %s", ec.message().c_str());
        return;
    }

    // Start the waveform sequence
    drv2605->start(ec);
    if (ec) {
        ESP_LOGE(TAG, "Failed to start vibration: %s", ec.message().c_str());
        return;
    }

    ESP_LOGI(TAG, "Vibration pattern %d played successfully", (int)pattern);
}

void drv2605_haptic_stop() {
    if (!drv2605 || !haptic_initialized) {
        ESP_LOGW(TAG, "DRV2605 not initialized");
        return;
    }
    
    std::error_code ec;
    drv2605->stop(ec);
    if (ec) {
        ESP_LOGE(TAG, "Failed to stop vibration: %s", ec.message().c_str());
    } else {
        ESP_LOGI(TAG, "Vibration stopped");
    }
}

bool drv2605_is_active() {
    // Simple check - return true if driver is initialized
    // Without access to hardware status, we can't determine actual playing state
    return (drv2605 != nullptr && haptic_initialized);
}

esp_err_t drv2605_haptic_driver_init() {
    ESP_LOGI(TAG, "Initializing DRV2605 haptic driver");
    
    esp_err_t ret = drv2605_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "DRV2605 initialization failed");
        haptic_initialized = false;
        return ret;
    }

    // Debug
    // while (true) {
    //     // Play a test vibration pattern
    //     drv2605_haptic_play_vibration(HAPTIC_ALERT_1000MS);
    //     ESP_LOGI(TAG, "DRV2605 haptic driver initialized and playing test pattern");
    //     vTaskDelay(pdMS_TO_TICKS(500)); // Keep the task alive to prevent deletion
    // }
    
    ESP_LOGI(TAG, "DRV2605 haptic driver initialized successfully");
    return ESP_OK;
}

void drv2605_haptic_driver_deinit() {
    ESP_LOGI(TAG, "Deinitializing DRV2605 haptic driver");
    
    if (drv2605) {
        std::error_code ec;
        drv2605->stop(ec);
        drv2605.reset(); // Release the smart pointer
    }
    
    haptic_initialized = false;
    
    #ifdef HAPTIC_EN
    gpio_set_level((gpio_num_t)HAPTIC_EN, 0); // Disable haptic driver power
    #endif
    
    ESP_LOGI(TAG, "DRV2605 haptic driver deinitialized");
}