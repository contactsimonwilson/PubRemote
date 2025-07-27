#include "vehicle_state.h"
#include "esp_log.h"
#include "esp_task.h"
#include "haptic/haptic_patterns.h"
#include "remote/haptic.h"
#include "remote/led.h"
#include "remote/stats.h"

static const char *TAG = "PUBREMOTE-VEHICLE_STATE";
#define VEHICLE_STATE_LOOP_TIME_MS 100

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

static void monitor_task(void *pvParameters) {
  DutyStatus last_duty_status = DUTY_STATUS_NONE;
  while (1) {
    DutyStatus current_duty_status = get_duty_status(remoteStats.dutyCycle);
    bool is_duty_alert = get_duty_status(remoteStats.dutyCycle) != DUTY_STATUS_NONE;
    if (current_duty_status != last_duty_status) {
      if (is_duty_alert) {
        ESP_LOGW(TAG, "Duty cycle alert: %d%%", remoteStats.dutyCycle);
        set_led_effect_solid(get_duty_color(current_duty_status));
        vibrate(HAPTIC_ALERT_1000MS);
        // Set buzzer
      }
      else {
        ESP_LOGI(TAG, "Duty cycle normal: %d%%", remoteStats.dutyCycle);
        set_led_effect_none();
        stop_vibration();
        // Reset buzzer
      }
      last_duty_status = current_duty_status;
      continue;
    }

    vTaskDelay(pdMS_TO_TICKS(VEHICLE_STATE_LOOP_TIME_MS));
  }
  vTaskDelete(NULL);
}

void init_vechicle_state_monitor() {
  xTaskCreate(monitor_task, "monitor_task", 4096, NULL, 5, NULL);
  ESP_LOGI("VEHICLE_STATE", "Vehicle state monitor initialized");
}
