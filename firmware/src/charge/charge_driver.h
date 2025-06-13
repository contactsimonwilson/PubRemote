#ifndef __CHARGE_DRIVER_H
#define __CHARGE_DRIVER_H
#include <esp_err.h>
#include <esp_lcd_types.h>

#ifndef PMIC_SDA
  #define PMIC_SDA -1
#endif
#ifndef PMIC_SCL
  #define PMIC_SCL -1
#endif
#ifndef PMIC_INT
  #define PMIC_INT -1
#endif

// Generic interface for display commands
esp_err_t charge_driver_init();
uint16_t get_battery_voltage();

#endif