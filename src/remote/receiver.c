#include "receiver.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "peers.h"
#include "time.h"

static const char *TAG = "PUBMOTE-RECEIVER";

static void on_data_recv(const uint8_t *mac_addr, const uint8_t *data, int len) {
  ESP_LOGI(TAG, "RECEIVED");
  int64_t deltaTime = get_current_time_ms() - LAST_COMMAND_TIME;
  LAST_COMMAND_TIME = 0;
  ESP_LOGI(TAG, "RTT: %lld", deltaTime);
}

void init_receiver() {
  ESP_LOGI(TAG, "Registered RX callback");
  ESP_ERROR_CHECK(esp_now_register_recv_cb(on_data_recv));
}