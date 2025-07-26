#include "imu_driver_qmi8658.hpp"
#include "esp_log.h"
#include "esp_err.h"
#define CONFIG_SENSORLIB_ESP_IDF_NEW_API
#include "SensorQMI8658.hpp"
#include "remote/i2c.h"
#include <driver/gpio.h>

static const char *TAG = "PUBREMOTE-IMU_DRIVER_QMI8658";

// QMI8658 I2C address
#ifndef QMI8658_ADDR
    #define QMI8658_ADDR                0x6B
#endif

// #define QMI8658_L_SLAVE_ADDRESS                 (0x6B)
// #define QMI8658_H_SLAVE_ADDRESS                 (0x6A)

static SensorQMI8658 imu;
static bool imu_initialized = false;
static IMUdata acc;
static IMUdata gyr;

static bool qmi8658_write_reg(uint8_t reg_addr, const uint8_t *data, size_t len)
{
    esp_err_t result = i2c_write_with_mutex(QMI8658_ADDR, reg_addr, (uint8_t*)data, len, 500);
    return (result == ESP_OK);
}

bool qmi8658_read_reg(uint8_t device_addr, uint8_t reg_addr, uint8_t* data, size_t len) {
    esp_err_t result = i2c_read_with_mutex(device_addr, reg_addr, data, len, 500);
    return (result == ESP_OK);
}
static bool qmi8658_reg_cb(uint8_t addr, uint8_t reg, uint8_t *buf, size_t len, bool writeReg, bool isWrite) {
    if (isWrite) {
        return qmi8658_write_reg(reg, buf, len);
    } else {
        return qmi8658_read_reg(addr, reg, buf, len);
    }
}

static esp_err_t qmi8658_init()
{
    #ifdef IMU_EN
    // Enable pin configuration
    gpio_reset_pin((gpio_num_t)IMU_EN);
    gpio_set_direction((gpio_num_t)IMU_EN, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)IMU_EN, 1); // Enable haptic driver power
    ESP_LOGI(TAG, "Haptic enable pin configured and activated");
    
    // Small delay to allow power stabilization
    vTaskDelay(pdMS_TO_TICKS(50));
    #endif

    // begin QMI8658 sensor
    imu.begin(qmi8658_reg_cb);

    return ESP_OK;
}

bool qmi8658_is_active() {
    // Simple check - return true if driver is initialized
    // Without access to hardware status, we can't determine actual playing state
    return  imu_initialized;
}

esp_err_t qmi8658_imu_driver_init() {
    ESP_LOGI(TAG, "Initializing QMI8658 IMU driver");
    
    esp_err_t ret = qmi8658_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "QMI8658 initialization failed");
        imu_initialized = false;
        return ret;
    }
    
    ESP_LOGI(TAG, "QMI8658 IMU driver initialized successfully");
    return ESP_OK;
}

void drv2605_imu_driver_deinit() {
    ESP_LOGI(TAG, "Deinitializing QMI8658 IMU driver");
    
    imu.stop();
    imu_initialized = false;
    
    #ifdef IMU_EN
    gpio_set_level((gpio_num_t)IMU_EN, 0); // Disable haptic driver power
    #endif
    
    ESP_LOGI(TAG, "QMI8658 IMU driver deinitialized");
}