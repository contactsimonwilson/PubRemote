#include "esp_lcd_panel_io.h"
#include <esp_err.h>
#include <esp_lcd_types.h>
#include <esp_log.h>

static const char *TAG = "PUBREMOTE-DISPLAY-DRIVER";

#if DISP_GC9A01
#elif DISP_SH8601
  #include "display_driver_sh8601.h"
#endif

esp_err_t test_display_communication(esp_lcd_panel_io_handle_t io_handle) {
#if DISP_GC9A01
  ESP_LOGI(TAG, "Testing GC9A01 communication");
  return ESP_OK;

#elif DISP_SH8601
  ESP_LOGI(TAG, "Testing SH8601Z communication");

  // Try to read display ID - should return non-zero values if SPI is working
  uint8_t id1[1] = {0};
  uint8_t id2[1] = {0};
  uint8_t id3[1] = {0};

  esp_lcd_panel_io_rx_param(io_handle, SH8601_R_RDID1, &id1, 1);
  esp_lcd_panel_io_rx_param(io_handle, SH8601_R_RDID2, &id2, 1);
  esp_lcd_panel_io_rx_param(io_handle, SH8601_R_RDID3, &id3, 1);

  ESP_LOGI(TAG, "Display ID: %02X %02X %02X", id1[0], id2[0], id3[0]);

  // Try to read power mode
  uint8_t power_mode = 0;
  esp_lcd_panel_io_rx_param(io_handle, SH8601_R_RDPOWERMODE, &power_mode, 1);
  ESP_LOGI(TAG, "Power mode: 0x%02X", power_mode);
  return ESP_OK;

#endif
}