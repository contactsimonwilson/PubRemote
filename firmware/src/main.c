#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "remote/adc.h"
#include "remote/buzzer.h"
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
  init_espnow();

  uint8_t mac[6];
  ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_STA, mac));
  esp_now_peer_info_t peerInfo = {};
  peerInfo.channel = 1; // Set the channel number (0-14)
  peerInfo.encrypt = false;
  memcpy(peerInfo.peer_addr, pairing_settings.remote_addr, sizeof(pairing_settings.remote_addr));
  ESP_ERROR_CHECK(esp_now_add_peer(&peerInfo));
  // Log MAC address
  ESP_LOGI(TAG, "MAC Address: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  init_receiver();
  init_transmitter();
}
