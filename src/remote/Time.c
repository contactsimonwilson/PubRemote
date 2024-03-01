#include "Time.h"
#include "esp_timer.h"

int64_t get_current_time_ms() {
  return esp_timer_get_time() / 1000;
}