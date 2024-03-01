#include "receiver.h"
#include "esp_log.h"
#include "peers.h"
#include "time.h"

void on_data_recv(const uint8_t *mac_addr, const uint8_t *data, int len) {
  ESP_LOGI(RECEIVER_TAG, "RECEIVED");
  int64_t deltaTime = get_current_time_ms() - LAST_COMMAND_TIME;
  LAST_COMMAND_TIME = 0;
  ESP_LOGI(RECEIVER_TAG, "RTT: %lld", deltaTime);
}