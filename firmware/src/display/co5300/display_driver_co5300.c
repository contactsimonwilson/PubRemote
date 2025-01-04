#include "display_driver_co5300.h"
#include "driver/ledc.h"
#include <esp_lcd_co5300.h>
#include <esp_lcd_panel_io.h>
#include <esp_log.h>

static const char *TAG = "PUBREMOTE-CO5300";

// Adapted from display_driver_co5300.c
#define LCD_OPCODE_WRITE_CMD (0x02ULL)
#define LCD_OPCODE_READ_CMD (0x03ULL)
#define LCD_OPCODE_WRITE_COLOR (0x32ULL)
#define USE_QSPI_INTERFACE 1

// https://github.com/moononournation/Arduino_GFX/blob/d55760955d101ee7d9d276142cd6fb59f721496f/src/display/Arduino_CO5300.h
const co5300_lcd_init_cmd_t co5300_lcd_init_cmds[] = {
    //  {cmd, { data }, data_size, delay_ms}
    {CO5300_C_SLPOUT, (uint8_t[]){0x00}, 0, CO5300_SLPOUT_DELAY},
    {0xFE, (uint8_t[]){0x00}, 0, 0},
    {CO5300_W_SPIMODECTL, (uint8_t[]){0x80}, 1, 0},
    {CO5300_W_PIXFMT, (uint8_t[]){0x55}, 1, 0}, // Interface Pixel Format 16bit/pixel
    {CO5300_W_WCTRLD1, (uint8_t[]){0x20}, 1, 0},
    {CO5300_W_WDBRIGHTNESSVALHBM, (uint8_t[]){0xFF}, 1, 0}, // Brightness Control On and Display Dimming On
    {CO5300_C_DISPON, (uint8_t[]){0x00}, 0, 0},             // Brightness adjustment
    {CO5300_W_WDBRIGHTNESSVALNOR, (uint8_t[]){0xD0}, 1, 0}, // Brightness adjustment
    {CO5300_W_WCE, (uint8_t[]){0x00}, 1, 10},               // High contrast mode (Sunlight Readability Enhancement)
};

size_t co5300_get_lcd_init_cmds_size(void) {
  return sizeof(co5300_lcd_init_cmds) / sizeof(co5300_lcd_init_cmd_t);
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

esp_err_t co5300_test_display_communication(esp_lcd_panel_io_handle_t io_handle) {
  ESP_LOGI(TAG, "Testing CO5300Z communication");

  uint8_t id_array[3] = {0};

  rx_param(io_handle, CO5300_R_RDID, &id_array, 3);

  ESP_LOGI(TAG, "Display ID: %02X %02X %02X", id_array[0], id_array[1], id_array[2]);
  return ESP_OK;
}

esp_err_t co5300_display_driver_preinit() {
  ESP_LOGI(TAG, "Preinit CO5300 display driver");
  gpio_reset_pin(DISP_BL);
  gpio_set_direction(DISP_BL, GPIO_MODE_OUTPUT);
  ESP_ERROR_CHECK(gpio_set_level(DISP_BL, 1));
  return ESP_OK;
}

esp_err_t co5300_set_display_brightness(esp_lcd_panel_io_handle_t io_handle, uint8_t brightness) {
  // Create a buffer for the command and brightness value
  uint8_t data[1] = {brightness};

  // Send the command and brightness value over SPI
  return tx_param(io_handle, CO5300_W_WDBRIGHTNESSVALNOR, data, 1);
}
