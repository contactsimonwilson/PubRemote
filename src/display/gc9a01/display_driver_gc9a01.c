#include "display_driver_gc9a01.h"
#include "driver/ledc.h"
#include "esp_lcd_panel_io.h"
#include "hal/ledc_types.h"
#include <esp_err.h>
#include <esp_lcd_types.h>
#include <esp_log.h>

static const char *TAG = "PUBREMOTE-GC9A01";

esp_err_t gc9a01_test_display_communication(esp_lcd_panel_io_handle_t io_handle) {
  ESP_LOGI(TAG, "Testing GC9A01 communication");
  return ESP_OK;
}

esp_err_t gc9a01_set_display_brightness(esp_lcd_panel_io_handle_t io_handle, uint8_t brightness) {
  ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, brightness);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
  return ESP_OK;
}