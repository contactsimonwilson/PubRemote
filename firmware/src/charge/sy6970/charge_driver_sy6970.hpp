#ifndef __CHARGE_DRIVER_SY6970_HPP
#define __CHARGE_DRIVER_SY6970_HPP

#include <esp_err.h>
#include <charge/charge_driver.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PMIC_SDA
  #define PMIC_SDA -1
#endif
#ifndef PMIC_SCL
  #define PMIC_SCL -1
#endif
#ifndef PMIC_INT
  #define PMIC_INT -1
#endif

esp_err_t sy6970_charge_driver_init();
RemotePowerState sy6970_get_power_state();

#ifdef __cplusplus
}
#endif

#endif