#include "display_driver_sh8601.h"
#include <esp_lcd_sh8601.h>

// https://github.com/nishad2m8/T-Display-S3-DS-1.43-YT/blob/master/01-Auto-Gauge/PIO/lib/Arduino_GFX-1.3.7/src/display/Arduino_SH8601.h
const sh8601_lcd_init_cmd_t lcd_init_cmds[] = {
    //  {cmd, { data }, data_size, delay_ms}
    {SH8601_C_SLPOUT, (uint8_t[]){0x00}, 0, SH8601_SLPOUT_DELAY},
    {SH8601_C_NORON, (uint8_t[]){0x00}, 0, 0},
    {SH8601_C_INVOFF, (uint8_t[]){0x00}, 0, 0},
    {SH8601_W_PIXFMT, (uint8_t[]){0x05}, 1, 0}, // Interface Pixel Format 16bit/pixel
    {SH8601_C_DISPON, (uint8_t[]){0x00}, 0, 0},
    {SH8601_W_WCTRLD1, (uint8_t[]){0x28}, 1, 0},            // Brightness Control On and Display Dimming On
    {SH8601_W_WDBRIGHTNESSVALNOR, (uint8_t[]){0xFF}, 1, 0}, // Brightness adjustment
    {SH8601_W_WCE, (uint8_t[]){0x00}, 1, 10},               // High contrast mode (Sunlight Readability Enhancement)
};

size_t get_lcd_init_cmds_size(void) {
  return sizeof(lcd_init_cmds) / sizeof(sh8601_lcd_init_cmd_t);
}