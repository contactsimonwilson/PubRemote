#ifndef __ADC_H
#define __ADC_H
#include <esp_adc/adc_cali.h>
#include <math.h>

#define STICK_ADC_BITWIDTH ADC_BITWIDTH_12
#define STICK_MAX_VAL ((1 << STICK_ADC_BITWIDTH) - 1)
#define STICK_MID_VAL ((STICK_MAX_VAL / 2) - 1)
#define STICK_MIN_VAL 0
#define STICK_DEADBAND 100
#define STICK_EXPO 1

bool adc_calibration_init(adc_unit_t unit, adc_atten_t atten, adc_cali_handle_t *out_handle);
#endif