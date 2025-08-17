#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "remote/adc.h"
#include "remote/buzzer.h"
#include "remote/connection.h"
#include "remote/console.h"
#include "remote/display.h"
#include "remote/espnow.h"
#include "remote/haptic.h"
#include "remote/i2c.h"
#include "remote/led.h"
#include "remote/peers.h"
#include "remote/powermanagement.h"
#include "remote/receiver.h"
#include "remote/remoteinputs.h"
#include "remote/screen.h"
#include "remote/settings.h"
#include "remote/startup.h"
#include "remote/stats.h"
#include "remote/time.h"
#include "remote/transmitter.h"
#include "remote/vehicle_state.h"
#include "ui/ui.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "PUBREMOTE-MAIN";

void app_main(void) {
  acc_power_enable(true);
  // Core setup
  init_i2c();
  settings_init();
  init_adcs();
  buttons_init(); // Required before power management for boot button detection
  buzzer_init();  // Required before power management for buzzer control
  haptic_init();  // Required before power management for haptic control
  power_management_init();

  // Peripherals
  led_init();
  thumbstick_init();
  display_init();
  vehicle_monitor_init();

  // Comms
  espnow_init();
  connection_init();
  receiver_init();
  transmitter_init();
  console_init();

  startup_cb();

  ESP_LOGI(TAG, "Boot complete");
}
