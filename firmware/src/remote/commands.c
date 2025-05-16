#include "commands.h"
#include "connection.h"
#include "powermanagement.h"
#include "settings.h"
#include "stats.h"
#include "time.h"
#include "utilities/conversion_utils.h"
#include <math.h>
#include <string.h>

static const char *TAG = "PUBREMOTE-COMMANDS";

bool process_board_data(uint8_t *data, int len) {
  if ((connection_state == CONNECTION_STATE_CONNECTED || connection_state == CONNECTION_STATE_RECONNECTING ||
       connection_state == CONNECTION_STATE_CONNECTING) &&
      len == 32) {
    reset_sleep_timer();
    remoteStats.lastUpdated = get_current_time_ms();
    int32_t super_secret_code = (int32_t)((data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]);

    if (super_secret_code == pairing_settings.secret_code) {
      uint8_t fault_code = data[4];
      float pitch_angle = (int16_t)((data[5] << 8) | data[6]) / 10.0;
      float roll_angle = (int16_t)((data[7] << 8) | data[8]) / 10.0;
      float input_voltage_filtered = (int16_t)((data[7] << 8) | data[8]) / 10.0;
      int16_t rpm = (int16_t)((data[13] << 8) | data[14]);
      float tot_current = (int16_t)((data[17] << 8) | data[18]) / 10.0;

      // Get RemoteStats
      float speed = (int16_t)((data[15] << 8) | data[16]) / 10.0;
      remoteStats.speed = convert_ms_to_kph(fabs(speed));

      float battery_level = (float)data[30] / 2.0;

      remoteStats.batteryPercentage = battery_level;
      remoteStats.batteryVoltage = input_voltage_filtered;
      float duty_cycle_now = (float)data[19] / 100.0 - 0.5;
      remoteStats.dutyCycle = (uint8_t)(fabs(duty_cycle_now) * 100);

      float motor_temp_filtered = (float)data[25] / 2.0;
      remoteStats.motorTemp = motor_temp_filtered;

      float fet_temp_filtered = (float)data[24] / 2.0;
      remoteStats.controllerTemp = fet_temp_filtered;

      uint8_t state = data[9];
      remoteStats.state = state;
      uint8_t switch_state = data[10];
      remoteStats.switchState = switch_state;

      float distance_abs;
      memcpy(&distance_abs, &data[20], sizeof(float));
      remoteStats.tripDistance = distance_abs;
      uint32_t odometer = (uint32_t)((data[26] << 24) | (data[27] << 16) | (data[28] << 8) | data[29]);

      // Print the extracted values
      // ESP_LOGI(TAG, "Mode: %d", mode);
      // ESP_LOGI(TAG, "Fault Code: %d", fault_code);
      // ESP_LOGI(TAG, "Pitch Angle: %.1f", pitch_angle);
      // ESP_LOGI(TAG, "Roll Angle: %.1f", roll_angle);
      // ESP_LOGI(TAG, "State: %d", state);
      // ESP_LOGI(TAG, "Switch State: %d", switch_state);
      // ESP_LOGI(TAG, "Input Voltage Filtered: %.1f", input_voltage_filtered);
      // ESP_LOGI(TAG, "RPM: %d", rpm);
      // ESP_LOGI(TAG, "Speed: %.1f", speed);
      // ESP_LOGI(TAG, "Total Current: %.1f", tot_current);
      // ESP_LOGI(TAG, "Duty Cycle Now: %.2f", duty_cycle_now);
      // ESP_LOGI(TAG, "Distance Absolute: %.2f", distance_abs);
      // ESP_LOGI(TAG, "FET Temperature Filtered: %.1f", fet_temp_filtered);
      // ESP_LOGI(TAG, "Motor Temperature Filtered: %.1f", motor_temp_filtered);
      // ESP_LOGI(TAG, "Odometer: %lu", odometer);
      // ESP_LOGI(TAG, "Battery Level: %.1f", battery_level);

      update_stats_display(); // TODO - use callbacks to update the UI instead of direct calls
      return true;
    }
    else {
      ESP_LOGE(TAG, "Super secret code mismatch: %li != %li", super_secret_code, pairing_settings.secret_code);
      return false; // Secret code mismatch
    }
  }
  return false;
}