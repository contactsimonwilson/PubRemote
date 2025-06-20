#ifndef __CHARGE_DRIVER_SY6970_HPP
#define __CHARGE_DRIVER_SY6970_HPP

#include <esp_err.h>
#include <charge/charge_driver.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t sy6970_charge_driver_init();
RemotePowerState sy6970_get_power_state();

#ifdef __cplusplus
}
#endif

#endif