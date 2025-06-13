#ifndef __CHARGE_DRIVER_ADC_H
#define __CHARGE_DRIVER_ADC_H
#include <esp_err.h>

#ifndef BAT_ADC // Note: must be ADC1
  #define BAT_ADC -1
#endif
#ifndef BAT_ADC_F
  #define BAT_ADC_F 0
#endif

esp_err_t adc_charge_driver_init();
uint16_t adc_get_battery_voltage();

#endif