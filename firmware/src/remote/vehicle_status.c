#include "vehicle_status.h"

DutyStatus get_duty_status(uint8_t duty) {
  if (duty >= DUTY_THRESHOLD_CRITICAL) {
    return DUTY_STATUS_CRITICAL;
  }
  else if (duty >= DUTY_THRESHOLD_WARNING) {
    return DUTY_STATUS_WARNING;
  }
  else if (duty >= DUTY_THRESHOLD_CAUTION) {
    return DUTY_STATUS_CAUTION;
  }
  else {
    return DUTY_STATUS_NONE;
  }
}

DutyStatusColor get_duty_color(DutyStatus status) {
  switch (status) {
  case DUTY_STATUS_CAUTION:
    return DUTY_COLOR_CAUTION;
  case DUTY_STATUS_WARNING:
    return DUTY_COLOR_WARNING;
  case DUTY_STATUS_CRITICAL:
    return DUTY_COLOR_CRITICAL;
  default:
    return DUTY_COLOR_NONE; // Don't use this
  }
}
