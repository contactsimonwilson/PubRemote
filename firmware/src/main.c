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
#include "remote/display.h"
#include "remote/espnow.h"
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
  init_adcs();
  init_settings();
  init_power_management();
  init_led();
  init_buzzer();
  init_buttons();
  init_thumbstick();
  init_display();

  // Init radio comms
  init_espnow();
  init_connection();
  init_receiver();
  init_transmitter();
}
