#include <cmath>
#include "imu_driver_qmi8658.hpp"
#include "esp_log.h"
#include "esp_err.h"
#define CONFIG_SENSORLIB_ESP_IDF_NEW_API
#include "SensorQMI8658.hpp"
#include "remote/i2c.h"
#include <driver/gpio.h>
#include "config.h"
#include "imu/imu_datatypes.h"

static const char *TAG = "PUBREMOTE-IMU_DRIVER_QMI8658";

// QMI8658 I2C address is either 0x6A or 0x6B depending on the state of the SA0 pin

static SensorQMI8658 imu;
static bool imu_initialized = false;

static bool qmi8658_write_reg(uint8_t reg_addr, const uint8_t *data, size_t len)
{
    esp_err_t result = i2c_write_with_mutex(QMI8658_ADDR, reg_addr, (uint8_t*)data, len, 500);
    return (result == ESP_OK);
}

static bool qmi8658_read_reg(uint8_t device_addr, uint8_t reg_addr, uint8_t* data, size_t len) {
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

static uint32_t hal_callback(SensorCommCustomHal::Operation op, void *param1, void *param2)
{
    switch (op) {
        case SensorCommCustomHal::OP_PINMODE:
        {
            uint8_t pin = reinterpret_cast<uintptr_t>(param1);
            uint8_t mode = reinterpret_cast<uintptr_t>(param2);
            
            // Convert Arduino pin modes to ESP-IDF GPIO modes
            gpio_mode_t gpio_mode;
            switch (mode) {
                case 0: // INPUT
                    gpio_mode = GPIO_MODE_INPUT;
                    gpio_set_direction((gpio_num_t)pin, gpio_mode);
                    gpio_set_pull_mode((gpio_num_t)pin, GPIO_PULLUP_ONLY);
                    break;
                case 1: // OUTPUT
                    gpio_mode = GPIO_MODE_OUTPUT;
                    gpio_set_direction((gpio_num_t)pin, gpio_mode);
                    break;
                case 2: // INPUT_PULLUP
                    gpio_mode = GPIO_MODE_INPUT;
                    gpio_set_direction((gpio_num_t)pin, gpio_mode);
                    gpio_set_pull_mode((gpio_num_t)pin, GPIO_PULLUP_ONLY);
                    break;
                default:
                    gpio_mode = GPIO_MODE_INPUT;
                    gpio_set_direction((gpio_num_t)pin, gpio_mode);
                    break;
            }
            break;
        }
        
        case SensorCommCustomHal::OP_DIGITALWRITE:
        {
            uint8_t pin = reinterpret_cast<uintptr_t>(param1);
            uint8_t level = reinterpret_cast<uintptr_t>(param2);
            gpio_set_level((gpio_num_t)pin, level);
            break;
        }
        
        case SensorCommCustomHal::OP_DIGITALREAD:
        {
            uint8_t pin = reinterpret_cast<uintptr_t>(param1);
            return gpio_get_level((gpio_num_t)pin);
        }
        
        case SensorCommCustomHal::OP_MILLIS:
            return (uint32_t)(esp_timer_get_time() / 1000);
            
        case SensorCommCustomHal::OP_DELAY:
        {
            if (param1) {
                uint32_t ms = reinterpret_cast<uintptr_t>(param1);
                vTaskDelay(pdMS_TO_TICKS(ms));
            }
            break;
        }
        
        case SensorCommCustomHal::OP_DELAYMICROSECONDS:
        {
            uint32_t us = reinterpret_cast<uintptr_t>(param1);
            esp_rom_delay_us(us);
            break;
        }
        
        default:
            break;
    }
    return 0;
}


static esp_err_t qmi8658_init()
{
    #ifdef IMU_EN
    // Enable pin configuration
    gpio_reset_pin((gpio_num_t)IMU_EN);
    gpio_set_direction((gpio_num_t)IMU_EN, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)IMU_EN, 1); // Activate IMU enable pin
    ESP_LOGI(TAG, "IMU enable pin configured and activated");
    
    // Small delay to allow power stabilization
    vTaskDelay(pdMS_TO_TICKS(50));
    #endif

    // begin QMI8658 sensor
    bool success = imu.begin(qmi8658_reg_cb, hal_callback, QMI8658_ADDR);
    if (!success) {
        ESP_LOGE(TAG, "Failed to initialize QMI8658 sensor");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "QMI8658 I2C communication established. Device ID: 0x%02X", imu.getChipID());

    success = imu.enableAccelerometer();
    if (!success) {
        ESP_LOGE(TAG, "Failed to enable accelerometer");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Accelerometer enabled");

    success = imu.enableGyroscope();
    if (!success) {
        ESP_LOGE(TAG, "Failed to enable gyroscope");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Gyroscope enabled");

    success = imu.configWakeOnMotion();
    if (!success) {
        ESP_LOGE(TAG, "Failed to configure QMI8658 Wake-On-Motion");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "QMI8658 Wake-On-Motion configured");

    return ESP_OK;
}

bool qmi8658_is_active() {
    // Simple check - return true if driver is initialized
    // Without access to hardware status, we can't determine actual playing state
    return  imu_initialized;
}

void qmi8658_get_data(imu_data_t *data) {
    if (!imu_initialized) {
        ESP_LOGW(TAG, "IMU not initialized");
        return;
    }

    if (data == nullptr) {
        ESP_LOGE(TAG, "Invalid data pointer");
        return;
    }

    uint8_t status =  imu.getStatusRegister();

    if (status) {
    switch (status) {
        case  SensorQMI8658::EVENT_WOM_MOTION:
            data->event = IMU_EVENT_WOM_MOTION;
            break;
        case  SensorQMI8658::EVENT_TAP_MOTION:
            data->event = IMU_EVENT_TAP;
            break;
        default:
            data->event = IMU_EVENT_NONE;
            break;
    }
} else {
    data->event = IMU_EVENT_NONE;
}

    IMUdata acc, gyr;
    // Read accelerometer data
    imu.getAccelerometer(acc.x, acc.y, acc.z);
    data->accel_x = acc.x;
    data->accel_y = acc.y;
    data->accel_z = acc.z;

    // Read gyroscope data
    imu.getGyroscope(gyr.x, gyr.y, gyr.z);
    data->gyro_x = gyr.x;
    data->gyro_y = gyr.y;
    data->gyro_z = gyr.z;
}



esp_err_t qmi8658_imu_driver_init() {
    ESP_LOGI(TAG, "Initializing QMI8658 IMU driver");
    
    esp_err_t ret = qmi8658_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "QMI8658 initialization failed");
        imu_initialized = false;
        return ret;
    }

    imu_initialized = true;
    
    ESP_LOGI(TAG, "QMI8658 IMU driver initialized successfully");
    return ESP_OK;
}

void qmi8658_imu_driver_deinit() {
    ESP_LOGI(TAG, "Deinitializing QMI8658 IMU driver");
    
    imu.reset(false);
    imu_initialized = false;
    
    #ifdef IMU_EN
    gpio_set_level((gpio_num_t)IMU_EN, 0); // Disable IMU
    #endif
    
    ESP_LOGI(TAG, "QMI8658 IMU driver deinitialized");
}