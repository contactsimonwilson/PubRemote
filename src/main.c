#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "remote/display.h"
#include "remote/espnow.h"
#include "remote/peers.h"
#include "remote/receiver.h"
#include "remote/remote.h"
#include "remote/remoteinputs.h"
#include "remote/router.h"
#include "remote/screen.h"
#include "remote/time.h"
#include "remote/transmitter.h"
#include "ui/ui.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "PUBMOTE-MAIN";

uint8_t PEER_MAC_ADDRESS[6] = {72, 49, 183, 171, 63, 137};
int64_t LAST_COMMAND_TIME = 0;

void app_main(void) {
  init_display();
  init_espnow();

  uint8_t mac[6];
  ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_STA, mac));
  esp_now_peer_info_t peerInfo = {};
  peerInfo.channel = 1; // Set the channel number (0-14)
  peerInfo.encrypt = false;
  uint8_t PEER_MAC_ADDRESS2[6] = {72, 49, 183, 171, 63, 137};
  memcpy(peerInfo.peer_addr, PEER_MAC_ADDRESS2, sizeof(PEER_MAC_ADDRESS2));
  ESP_ERROR_CHECK(esp_now_add_peer(&peerInfo));
  // Log MAC address
  ESP_LOGI(TAG, "MAC Address: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  init_receiver();
  init_transmitter();
  // Remote inputs init
  init_buttons();
  init_throttle();
  RemoteScreen stats_screen = {.name = "stats", .screen_obj = ui_StatsScreen};
  RemoteScreen calibration_screen = {.name = "calibration", .screen_obj = ui_CalibrationScreen};
  router_register_screen(&stats_screen);
  router_register_screen(&calibration_screen);
  // router_show_screen("calibration");
}
