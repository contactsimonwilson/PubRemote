#include "imu_driver.h"
#include "esp_log.h"
#if IMU_QMI8658
  #include "imu/qmi8658/imu_driver_qmi8658.hpp"
#endif

static const char *TAG = "PUBREMOTE-IMU";

esp_err_t imu_driver_init() {
#if IMU_QMI8658
  // Initialize the DRV2605 haptic driver
  ESP_LOGI(TAG, "Initializing QMI8568 haptic driver");
  return qmi8658_imu_driver_init();
#elif IMU_BHI260
  return ESP_ERR_NOT_SUPPORTED; // BHI260 not implemented yet
                                // // Initialize the BHI260 haptic driver
                                // ESP_LOGI(TAG, "Initializing BHI260 haptic driver");
                                // return bhi260_imu_driver_init();
#else
  ESP_LOGE(TAG, "No IMU driver defined");
  return ESP_ERR_NOT_SUPPORTED;
#endif
}
