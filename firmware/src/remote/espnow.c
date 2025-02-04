#include "connection.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "settings.h"
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <nvs.h>
#include <remote/stats.h>

static const char *TAG = "PUBREMOTE-ESPNOW";

void init_espnow() {
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // Initialize TCP/IP,  event loop, and WiFi
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  ESP_ERROR_CHECK(esp_wifi_init(&cfg)); // You might not need a WiFi config
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_set_channel(pairing_settings.channel, WIFI_SECOND_CHAN_NONE));

  // Initialize ESP-NOW
  ESP_ERROR_CHECK(esp_now_init());
}