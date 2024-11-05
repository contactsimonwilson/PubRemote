#include "display_driver_sh8601.h"
#include <esp_lcd_panel_io.h>
#include <esp_lcd_sh8601.h>
#include <esp_log.h>

static const char *TAG = "PUBREMOTE-SH8601";

// Adapted from display_driver_sh8601.c
#define LCD_OPCODE_WRITE_CMD (0x02ULL)
#define LCD_OPCODE_READ_CMD (0x03ULL)
#define LCD_OPCODE_WRITE_COLOR (0x32ULL)
#define USE_QSPI_INTERFACE 1

// https://github.com/nishad2m8/T-Display-S3-DS-1.43-YT/blob/master/01-Auto-Gauge/PIO/lib/Arduino_GFX-1.3.7/src/display/Arduino_SH8601.h
const sh8601_lcd_init_cmd_t sh8601_lcd_init_cmds[] = {
    //  {cmd, { data }, data_size, delay_ms}
    {SH8601_C_SLPOUT, (uint8_t[]){0x00}, 0, SH8601_SLPOUT_DELAY},
    {SH8601_C_NORON, (uint8_t[]){0x00}, 0, 0},
    {SH8601_C_INVOFF, (uint8_t[]){0x00}, 0, 0},
    {SH8601_W_PIXFMT, (uint8_t[]){0x05}, 1, 0}, // Interface Pixel Format 16bit/pixel
    {SH8601_C_DISPON, (uint8_t[]){0x00}, 0, 0},
    {SH8601_W_WCTRLD1, (uint8_t[]){0x28}, 1, 0},            // Brightness Control On and Display Dimming On
    {SH8601_W_WDBRIGHTNESSVALNOR, (uint8_t[]){0x00}, 1, 0}, // Brightness adjustment
    {SH8601_W_WCE, (uint8_t[]){0x00}, 1, 10},               // High contrast mode (Sunlight Readability Enhancement)
};

size_t sh8601_get_lcd_init_cmds_size(void) {
  return sizeof(sh8601_lcd_init_cmds) / sizeof(sh8601_lcd_init_cmd_t);
}

static esp_err_t tx_param(esp_lcd_panel_io_handle_t io, int lcd_cmd, const void *param, size_t param_size) {
  if (USE_QSPI_INTERFACE) {
    lcd_cmd &= 0xff;
    lcd_cmd <<= 8;
    lcd_cmd |= LCD_OPCODE_WRITE_CMD << 24;
  }
  return esp_lcd_panel_io_tx_param(io, lcd_cmd, param, param_size);
}

static esp_err_t rx_param(esp_lcd_panel_io_handle_t io, int lcd_cmd, const void *param, size_t param_size) {
  if (USE_QSPI_INTERFACE) {
    lcd_cmd &= 0xff;
    lcd_cmd <<= 8;
    lcd_cmd |= LCD_OPCODE_READ_CMD << 24;
  }
  return esp_lcd_panel_io_tx_param(io, lcd_cmd, param, param_size);
}

esp_err_t sh8601_test_display_communication(esp_lcd_panel_io_handle_t io_handle) {
  ESP_LOGI(TAG, "Testing SH8601Z communication");

  uint8_t id_array[3] = {0};

  rx_param(io_handle, SH8601_R_RDID, &id_array, 3);

  ESP_LOGI(TAG, "Display ID: %02X %02X %02X", id_array[0], id_array[1], id_array[2]);
  return ESP_OK;
}

esp_err_t sh8601_set_display_brightness(esp_lcd_panel_io_handle_t io_handle, uint8_t brightness) {
  // Create a buffer for the command and brightness value
  uint8_t data[1] = {brightness};

  // Send the command and brightness value over SPI
  return tx_param(io_handle, SH8601_W_WDBRIGHTNESSVALNOR, data, 1);
}
