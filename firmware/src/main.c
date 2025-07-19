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
#include "remote/remote.h"
#include "remote/remoteinputs.h"
#include "remote/screen.h"
#include "remote/settings.h"
#include "remote/stats.h"
#include "remote/time.h"
#include "remote/transmitter.h"
#include "ui/ui.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "PUBREMOTE-MAIN";
int64_t LAST_COMMAND_TIME = 0;

void app_main(void) {
  // Core setup
  init_i2c();
  init_settings();
  init_adcs();
  init_buttons(); // Required before power management for boot button detection
  init_buzzer();  // Required before power management for buzzer control
  init_haptic();  // Required before power management for haptic control
  init_power_management();

  // Peripherals
  init_led();
  init_thumbstick();
  init_display();

  // Comms
  init_espnow();
  init_connection();
  init_receiver();
  init_transmitter();
  init_console();

  play_startup_sound();
  ESP_LOGI(TAG, "Boot complete");
}
