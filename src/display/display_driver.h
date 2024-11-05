#ifndef __DISPLAY_DRIVER_H
#define __DISPLAY_DRIVER_H
#include <esp_err.h>
#include <esp_lcd_types.h>

// Generic interface for display commands
esp_err_t test_display_communication(esp_lcd_panel_io_handle_t io_handle);
esp_err_t set_display_brightness(esp_lcd_panel_io_handle_t io_handle, uint8_t brightness);

#endif