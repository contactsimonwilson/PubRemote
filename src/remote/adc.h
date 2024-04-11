#ifndef __ADC_H
#define __ADC_H
#include <esp_adc/adc_cali.h>

bool adc_calibration_init(adc_unit_t unit, adc_atten_t atten, adc_cali_handle_t *out_handle);
#endif