#ifndef __CHARGE_DRIVER_ADC_H
#define __CHARGE_DRIVER_ADC_H
#include <esp_err.h>

esp_err_t adc_charge_driver_init();
uint16_t adc_get_battery_voltage();

#endif