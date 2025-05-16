#include "commands.h"
#include "connection.h"
#include "powermanagement.h"
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

    uint8_t fault_code = data[0];
    float pitch_angle = (int16_t)((data[1] << 8) | data[2]) / 10.0;
    float roll_angle = (int16_t)((data[3] << 8) | data[4]) / 10.0;
    float input_voltage_filtered = (int16_t)((data[7] << 8) | data[8]) / 10.0;
    int16_t rpm = (int16_t)((data[9] << 8) | data[10]);
    float tot_current = (int16_t)((data[13] << 8) | data[14]) / 10.0;

    // Get RemoteStats
    float speed = (int16_t)((data[11] << 8) | data[12]) / 10.0;
    remoteStats.speed = convert_ms_to_kph(fabs(speed));

    float battery_level = (float)data[26] / 2.0;

    remoteStats.batteryPercentage = battery_level;
    remoteStats.batteryVoltage = input_voltage_filtered;
    float duty_cycle_now = (float)data[15] / 100.0 - 0.5;
    remoteStats.dutyCycle = (uint8_t)(fabs(duty_cycle_now) * 100);

    float motor_temp_filtered = (float)data[21] / 2.0;
    remoteStats.motorTemp = motor_temp_filtered;

    float fet_temp_filtered = (float)data[20] / 2.0;
    remoteStats.controllerTemp = fet_temp_filtered;

    uint8_t state = data[5];
    remoteStats.state = state;
    uint8_t switch_state = data[6];
    remoteStats.switchState = switch_state;

    float distance_abs;
    memcpy(&distance_abs, &data[16], sizeof(float));
    remoteStats.tripDistance = distance_abs;
    uint32_t odometer = (uint32_t)((data[22] << 24) | (data[23] << 16) | (data[24] << 8) | data[25]);

    int32_t super_secret_code = (int32_t)((data[27] << 24) | (data[28] << 16) | (data[29] << 8) | data[30]);

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
  return false;
}