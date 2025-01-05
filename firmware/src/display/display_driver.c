#include "display_driver.h"
#include <esp_log.h>
static const char *TAG = "PUBREMOTE-DISPLAY-DRIVER";

#if DISP_GC9A01
  #include "gc9a01/display_driver_gc9a01.h"
#elif DISP_SH8601
  #include "sh8601/display_driver_sh8601.h"
#elif DISP_CO5300
  #include "sh8601/display_driver_sh8601.h"
#elif DISP_ST7789
  #include "st7789/display_driver_st7789.h"
#endif

esp_err_t test_display_communication(esp_lcd_panel_io_handle_t io_handle) {
  ESP_LOGI(TAG, "Testing display communication");
#if DISP_GC9A01
  return gc9a01_test_display_communication(io_handle);
#elif DISP_SH8601
  return sh8601_test_display_communication(io_handle);
#elif DISP_CO5300
  return sh8601_test_display_communication(io_handle);
#elif DISP_ST7789
  return st7789_test_display_communication(io_handle);
#endif
}

esp_err_t display_driver_preinit() {
  ESP_LOGI(TAG, "Preinit display driver");
#if DISP_GC9A01
  return gc9a01_display_driver_preinit();
#elif DISP_SH8601
  return sh8601_display_driver_preinit();
#elif DISP_CO5300
  return sh8601_display_driver_preinit();
#elif DISP_ST7789
  return st7789_display_driver_preinit();
#endif
}

esp_err_t set_display_brightness(esp_lcd_panel_io_handle_t io_handle, uint8_t brightness) {
  ESP_LOGI(TAG, "Setting display brightness");
#if DISP_GC9A01
  return gc9a01_set_display_brightness(io_handle, brightness);
#elif DISP_SH8601
  return sh8601_set_display_brightness(io_handle, brightness);
#elif DISP_CO5300
  return sh8601_set_display_brightness(io_handle, brightness);
#elif DISP_ST7789
  return st7789_set_display_brightness(io_handle, brightness);
#endif
}
