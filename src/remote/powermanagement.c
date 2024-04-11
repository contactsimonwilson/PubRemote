#include "adc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include <esp_adc/adc_oneshot.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <math.h>
#include <ui/ui.h>

static const char *TAG = "PUBMOTE-POWERMANAGEMENT";

float convert_adc_to_battery_volts(int adc_value) {
  // 0 - 4095 -> 0 - 255
  float val = 3.3 / (1 << 12) * 3 * adc_value;
  return roundf(val * 10) / 10;
}

float BATTERY_VOLTAGE = 0;
#define BATTER_MONITOR_CHANNEL ADC_CHANNEL_0 // Assuming the Hall sensor is connected to GPIO0

void power_management_task(void *pvParameters) {
  // Configure the ADC
  adc_oneshot_unit_handle_t adc1_handle;
  adc_oneshot_unit_init_cfg_t init_config = {
      .unit_id = ADC_UNIT_1,
      .ulp_mode = ADC_ULP_MODE_DISABLE,
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));

  // Calibration
  adc_cali_handle_t adc_cali_handle = NULL;
  bool do_calibration2 = adc_calibration_init(ADC_UNIT_1, ADC_ATTEN_DB_12, &adc_cali_handle);

  // Configure the ADC channel
  adc_oneshot_chan_cfg_t channel_config = {
      .bitwidth = ADC_BITWIDTH_12,
      .atten = ADC_ATTEN_DB_12,
  };
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, BATTER_MONITOR_CHANNEL, &channel_config));
  vTaskDelay(pdMS_TO_TICKS(1000));

  while (1) {
    int battery_value = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, BATTER_MONITOR_CHANNEL, &battery_value));
    BATTERY_VOLTAGE = convert_adc_to_battery_volts(battery_value);
    printf("Battery volts: %.1f\n", BATTERY_VOLTAGE);
    // char str[20];
    // sprintf(str, "%.1f", BATTERY_VOLTAGE);
    //  lv_label_set_text(ui_PrimaryStat, str);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  ESP_LOGI(TAG, "Power management task ended");
  // terminate self
  vTaskDelete(NULL);
}

void init_power_management() {
  xTaskCreate(power_management_task, "power_management_task", 4096, NULL, 2, NULL);
}