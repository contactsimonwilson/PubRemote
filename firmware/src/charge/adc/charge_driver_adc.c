#include "charge_driver_adc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_err.h>
#include <remote/adc.h>

static const char *TAG = "PUBREMOTE-CHARGE_DRIVER_ADC";

#ifndef BAT_ADC_F
  #define BAT_ADC_F 3
#endif

esp_err_t adc_charge_driver_init() {
  return adc_oneshot_config_channel(adc1_handle, BAT_ADC, &adc_channel_config);
}

uint16_t adc_get_battery_voltage() {
#define NUM_SAMPLES 3
  uint8_t num_successful_samples = 0;
  int32_t accumulated = 0;

  for (int i = 0; i < NUM_SAMPLES; i++) {
    int sample = 0;
    int voltage_mv = 0;
    esp_err_t res = ESP_OK;
    res = adc_oneshot_read(adc1_handle, BAT_ADC, &sample);

    if (res != ESP_OK) {
      ESP_LOGE(TAG, "Failed to read battery voltage.");
      continue;
    }

    res = adc_cali_raw_to_voltage(adc1_cali_handle, sample, &voltage_mv);

    if (res != ESP_OK) {
      ESP_LOGE(TAG, "Failed to convert raw value to voltage.");
      continue;
    }

    accumulated += voltage_mv;
    num_successful_samples++;
    vTaskDelay(pdMS_TO_TICKS(1));
  }

  if (num_successful_samples == 0) {
    ESP_LOGE(TAG, "Failed to read battery voltage.");
    return 0.0f;
  }

  int battery_value = accumulated / num_successful_samples;
  return (uint16_t)(battery_value * BAT_ADC_F);
}