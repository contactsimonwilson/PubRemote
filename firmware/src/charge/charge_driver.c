#include "charge_driver.h"
#include <esp_log.h>
static const char *TAG = "PUBREMOTE-CHARGE-DRIVER";

#if PMIC_SY6970
  #include "sy6970/charge_driver_sy6970.hpp"
#else
  #include "adc/charge_driver_adc.h"
#endif

esp_err_t charge_driver_init() {
  ESP_LOGI(TAG, "Init charge driver");
#if PMIC_SY6970
  return sy6970_charge_driver_init();
#else
  return adc_charge_driver_init();
#endif
}

uint16_t get_battery_voltage() {
  ESP_LOGD(TAG, "Getting battery voltage");
#if PMIC_SY6970
  return sy6970_get_battery_voltage();
#else
  return adc_get_battery_voltage();
#endif
}
