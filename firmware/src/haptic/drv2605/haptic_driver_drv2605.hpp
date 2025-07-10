#ifndef __HAPIC_DRIVER_DRV2605_HPP
#define __HAPIC_DRIVER_DRV2605_HPP

#include "remote/haptic.h"
#include <esp_err.h>

#ifdef __cplusplus
extern "C"
{
#endif

  void drv2605_haptic_play_vibration(HapticFeedbackPattern pattern);
  esp_err_t drv2605_haptic_driver_init();

#ifdef __cplusplus
}
#endif

#endif