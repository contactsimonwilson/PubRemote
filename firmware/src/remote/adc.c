#include "adc.h"
#include "esp_log.h"
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_err.h>
#include <hal/adc_types.h>
#include <remote/adc.h>

static const char *TAG = "PUBREMOTE-ADC";

const adc_oneshot_chan_cfg_t adc_channel_config = {
    .bitwidth = STICK_ADC_BITWIDTH,
    .atten = ADC_ATTEN_DB_12,
};

static const adc_oneshot_unit_init_cfg_t adc1_init_config = {
    .unit_id = ADC_UNIT_1,
    .ulp_mode = ADC_ULP_MODE_DISABLE,
};

static const adc_oneshot_unit_init_cfg_t adc2_init_config = {
    .unit_id = ADC_UNIT_2,
    .ulp_mode = ADC_ULP_MODE_DISABLE,
};

adc_oneshot_unit_handle_t adc1_handle = NULL;
adc_cali_handle_t adc1_cali_handle = NULL;

adc_oneshot_unit_handle_t adc2_handle = NULL;
adc_cali_handle_t adc2_cali_handle = NULL;

static bool adc_calibration_init(adc_unit_t unit, adc_atten_t atten, adc_cali_handle_t *out_handle) {
  adc_cali_handle_t handle = NULL;
  esp_err_t ret = ESP_FAIL;
  bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
  if (!calibrated) {
    ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = unit,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
    if (ret == ESP_OK) {
      calibrated = true;
    }
  }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
  if (!calibrated) {
    ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = unit,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
    if (ret == ESP_OK) {
      calibrated = true;
    }
  }
#endif

  *out_handle = handle;
  if (ret == ESP_OK) {
    ESP_LOGI(TAG, "Calibration Success");
  }
  else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
    ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
  }
  else {
    ESP_LOGE(TAG, "Invalid arg or no memory");
  }

  return calibrated;
}

void init_adcs() {
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc1_init_config, &adc1_handle));
  adc_calibration_init(adc1_init_config.unit_id, adc_channel_config.atten, &adc1_cali_handle);
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc2_init_config, &adc2_handle));
  adc_calibration_init(adc2_init_config.unit_id, adc_channel_config.atten, &adc2_cali_handle);
}