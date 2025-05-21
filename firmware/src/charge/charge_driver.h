#ifndef __CHARGE_DRIVER_H
#define __CHARGE_DRIVER_H
#include <esp_err.h>
#include <esp_lcd_types.h>

// Generic interface for display commands
esp_err_t charge_driver_init();
uint16_t get_battery_voltage();

#endif