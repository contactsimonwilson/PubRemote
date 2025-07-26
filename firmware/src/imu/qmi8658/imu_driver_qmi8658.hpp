#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t qmi8658_imu_driver_init(void);
void qmi8658_imu_driver_deinit(void);
bool qmi8658_is_active(void);

#ifdef __cplusplus
}
#endif