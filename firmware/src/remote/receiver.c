#include "receiver.h"
#include "connection.h"
#include "display.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "peers.h"
#include "powermanagement.h"
#include "screens/pairing_screen.h"
#include "stats.h"
#include "time.h"
#include "utilities/conversion_utils.h"
#include <freertos/queue.h>
#include <math.h>
#include <remote/settings.h>
#include <stdlib.h>
#include <string.h>
#include <ui/ui.h>

static const char *TAG = "PUBREMOTE-RECEIVER";
#define RX_QUEUE_SIZE 10

// Structure to hold ESP-NOW data
typedef struct {
  uint8_t mac_addr[ESP_NOW_ETH_ALEN];
  uint8_t *data;
  int len;
  uint8_t chan;
} esp_now_event_t;

static QueueHandle_t espnow_queue;

static void on_data_recv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
  // This callback runs in WiFi task context!
  ESP_LOGD(TAG, "RECEIVED");
  esp_now_event_t evt;
  memcpy(evt.mac_addr, recv_info->src_addr, ESP_NOW_ETH_ALEN);
  evt.data = malloc(len);
  memcpy(evt.data, data, len);
  evt.len = len;
  evt.chan = recv_info->rx_ctrl->channel;
  remoteStats.signalStrength = recv_info->rx_ctrl->rssi;

#if RX_QUEUE_SIZE > 1
  // Send to queue for processing in application task
  if (uxQueueSpacesAvailable(espnow_queue) == 0) {
    // reset the queue
    xQueueReset(espnow_queue);
  }
  if (xQueueSend(espnow_queue, &evt, portMAX_DELAY) != pdTRUE) {
#else
  // overwrite the previous data
  if (xQueueOverwrite(espnow_queue, &evt) != pdTRUE) {
#endif
    ESP_LOGE(TAG, "Queue send failed");
    free(evt.data);
  }
}

static bool is_same_mac(const uint8_t *mac1, const uint8_t *mac2) {
  return memcmp(mac1, mac2, ESP_NOW_ETH_ALEN) == 0;
}

static void process_data(esp_now_event_t evt) {
  uint8_t *data = evt.data;
  int len = evt.len;
  int64_t deltaTime = get_current_time_ms() - LAST_COMMAND_TIME;
  LAST_COMMAND_TIME = 0;
  ESP_LOGD(TAG, "RTT: %lld", deltaTime);

  bool is_pairing_start = pairing_state == PAIRING_STATE_UNPAIRED && is_pairing_screen_active();

  // Check mac for security on anything other than initial pairing
  if (!is_same_mac(evt.mac_addr, pairing_settings.remote_addr) && !is_pairing_start) {
    ESP_LOGD(TAG, "Ignoring data from unknown MAC");
    return;
  }

  if (is_pairing_start && len == 6) {
    uint8_t rec_mac[ESP_NOW_ETH_ALEN];
    memcpy(rec_mac, data, ESP_NOW_ETH_ALEN);
    if (!is_same_mac(evt.mac_addr, rec_mac)) {
      ESP_LOGE(TAG, "MAC Address mismatch on pairing request");
      return;
    }
    memcpy(pairing_settings.remote_addr, rec_mac, ESP_NOW_ETH_ALEN);
    ESP_LOGI(TAG, "Got Pairing request from VESC Express");
    ESP_LOGI(TAG, "packet Length: %d", len);
    ESP_LOGI(TAG, "MAC Address: %02X:%02X:%02X:%02X:%02X:%02X", data[0], data[1], data[2], data[3], data[4], data[5]);
    // ESP_LOGI(TAG, "Incorrect MAC Address: %02X:%02X:%02X:%02X:%02X:%02X", mac_addr[0], mac_addr[1], mac_addr[2],
    //          mac_addr[3], mac_addr[4], mac_addr[5]);
    uint8_t TEST[1] = {420}; // TODO - FIX THIS
    // Do this internally as we don't want it to change connection state
    esp_now_peer_info_t peerInfo = {};
    peerInfo.channel = evt.chan; // Set the channel number (0-14)
    peerInfo.encrypt = false;
    memcpy(peerInfo.peer_addr, pairing_settings.remote_addr, sizeof(pairing_settings.remote_addr));
    pairing_settings.channel = evt.chan;
    // ESP_ERROR_CHECK(esp_now_add_peer(&peerInfo));
    uint8_t *mac_addr = pairing_settings.remote_addr;
    esp_err_t result = ESP_FAIL;

    if (channel_lock()) {
      if (esp_now_is_peer_exist(mac_addr)) {
        esp_err_t res = esp_now_del_peer(mac_addr);
        if (res != ESP_OK) {
          ESP_LOGE(TAG, "Failed to delete peer");
        }
      }

      esp_now_add_peer(&peerInfo);

      result = esp_now_send(mac_addr, (uint8_t *)&TEST, sizeof(TEST));
      channel_unlock();
    }

    if (result != ESP_OK) {
      // Handle error if needed
      ESP_LOGE(TAG, "Error sending pairing data: %d", result);
    }
    else {
      ESP_LOGI(TAG, "Sent response back to VESC Express");
      pairing_state = PAIRING_STATE_PAIRING;
    }
  }
  else if (pairing_state == PAIRING_STATE_PAIRING && len == 4) {
    // grab secret code
    ESP_LOGI(TAG, "Grabbing secret code");
    ESP_LOGI(TAG, "packet Length: %d", len);
    pairing_settings.secret_code = (int32_t)(data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
    ESP_LOGI(TAG, "Secret Code: %li", pairing_settings.secret_code);
    char *formattedString;
    asprintf(&formattedString, "%ld", pairing_settings.secret_code);
    if (LVGL_lock(0)) {
      lv_label_set_text(ui_PairingCode, formattedString);
      LVGL_unlock();
    }
    free(formattedString);
    pairing_state = PAIRING_STATE_PENDING;
  }
  else if (pairing_state == PAIRING_STATE_PENDING && len == 4) {
    // grab secret code
    ESP_LOGI(TAG, "Grabbing response");
    ESP_LOGI(TAG, "packet Length: %d", len);
    int response = (int32_t)(data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
    ESP_LOGI(TAG, "Response: %i", response);
    if (LVGL_lock(0)) {
      if (response == -1) {
        pairing_state = PAIRING_STATE_PAIRED;
        save_pairing_data();
        connect_to_default_peer();
        lv_disp_load_scr(ui_StatsScreen);
      }
      else {
        pairing_state = PAIRING_STATE_UNPAIRED;
      }
      LVGL_unlock();
    }
  }
  else if ((connection_state == CONNECTION_STATE_CONNECTED || connection_state == CONNECTION_STATE_RECONNECTING ||
            connection_state == CONNECTION_STATE_CONNECTING) &&
           len == 32) {
    reset_sleep_timer();
    remoteStats.lastUpdated = get_current_time_ms();

    uint8_t mode = data[0];
    uint8_t fault_code = data[1];
    float pitch_angle = (int16_t)((data[2] << 8) | data[3]) / 10.0;
    float roll_angle = (int16_t)((data[4] << 8) | data[5]) / 10.0;
    float input_voltage_filtered = (int16_t)((data[8] << 8) | data[9]) / 10.0;
    int16_t rpm = (int16_t)((data[10] << 8) | data[11]);
    float tot_current = (int16_t)((data[14] << 8) | data[15]) / 10.0;

    // Get RemoteStats
    float speed = (int16_t)((data[12] << 8) | data[13]) / 10.0;
    remoteStats.speed = convert_ms_to_kph(fabs(speed));

    float battery_level = (float)data[27] / 2.0;

    remoteStats.batteryPercentage = battery_level;
    float duty_cycle_now = (float)data[16] / 100.0 - 0.5;
    remoteStats.dutyCycle = (uint8_t)(fabs(duty_cycle_now) * 100);

    float motor_temp_filtered = (float)data[22] / 2.0;
    remoteStats.motorTemp = motor_temp_filtered;

    float fet_temp_filtered = (float)data[21] / 2.0;
    remoteStats.controllerTemp = fet_temp_filtered;

    uint8_t state = data[6];
    uint8_t switch_state = data[7];
    remoteStats.switchState = switch_state;

    float distance_abs;
    memcpy(&distance_abs, &data[17], sizeof(float));
    uint32_t odometer = (uint32_t)((data[23] << 24) | (data[24] << 16) | (data[25] << 8) | data[26]);

    int32_t super_secret_code = (int32_t)((data[28] << 24) | (data[29] << 16) | (data[30] << 8) | data[31]);

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
  }
  else if (connection_state == CONNECTION_STATE_DISCONNECTED) {
    // Do nothing
  }
  else {
    ESP_LOGI(TAG, "Invalid data length %d", len);
  }
}

#define CHANNEL_HOP_INTERVAL_MS 200
#define RECEIVER_TASK_DELAY_MS 5

// Mutex to protect channel switching
static SemaphoreHandle_t channel_mutex;

bool channel_lock() {
  if (channel_mutex == NULL) {
    channel_mutex = xSemaphoreCreateMutex();
  }
  return xSemaphoreTake(channel_mutex, pdMS_TO_TICKS(1000)) == pdTRUE;
}

void channel_unlock() {
  if (channel_mutex == NULL) {
    channel_mutex = xSemaphoreCreateMutex();
  }

  xSemaphoreGive(channel_mutex);
}

static void change_channel(uint8_t chan, bool is_pairing) {
  ESP_LOGI(TAG, "Switching to channel %d", chan);
  channel_lock();

  esp_wifi_set_channel(chan, WIFI_SECOND_CHAN_NONE);
  pairing_settings.channel = chan;

  if (!is_pairing) {
    // Add peer so we can send if we're already paired
    uint8_t *mac_addr = pairing_settings.remote_addr;

    if (esp_now_is_peer_exist(mac_addr)) {
      esp_now_del_peer(mac_addr);
    }

    esp_now_peer_info_t peerInfo = {};
    peerInfo.channel = chan;
    peerInfo.encrypt = false;
    memcpy(peerInfo.peer_addr, mac_addr, ESP_NOW_ETH_ALEN);
    esp_now_add_peer(&peerInfo);
  }

  channel_unlock();
}

static void receiver_task(void *pvParameters) {
  espnow_queue = xQueueCreate(RX_QUEUE_SIZE, sizeof(esp_now_event_t));
  ESP_ERROR_CHECK(esp_now_register_recv_cb(on_data_recv));
  ESP_LOGI(TAG, "Registered RX callback");
  esp_now_event_t evt;
  // Hop through channels if in pairing mode or connecting
  uint64_t channel_switch_time_ms = 0;

  while (1) {
    if (xQueueReceive(espnow_queue, &evt, 0) == pdTRUE) {
      process_data(evt);
      free(evt.data);
      // reset channel switch time
      channel_switch_time_ms = 0;
    }
    else {
      bool is_pairing = pairing_state == PAIRING_STATE_UNPAIRED && is_pairing_screen_active();
      bool is_connecting = connection_state == CONNECTION_STATE_CONNECTING;
      // Nothing received while connecting or pairing - hop through channels
      if (is_connecting || is_pairing) {
        if (channel_switch_time_ms > CHANNEL_HOP_INTERVAL_MS) {
// Hop to next channel
#define NUM_AVAIL_WIFI_CHANNELS 14

          uint8_t next_channel = (pairing_settings.channel % NUM_AVAIL_WIFI_CHANNELS) + 1;
          change_channel(next_channel, is_pairing);
          channel_switch_time_ms = 0;
        }
        else {
          channel_switch_time_ms += RECEIVER_TASK_DELAY_MS;
        }
      }
      else {
        // reset channel switch time
        channel_switch_time_ms = 0;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(RECEIVER_TASK_DELAY_MS));
  }

  // The task will not reach this point as it runs indefinitely
  ESP_LOGI(TAG, "RX task ended");
  vTaskDelete(NULL);
}

void init_receiver() {
  ESP_LOGI(TAG, "Starting receiver task");
  xTaskCreatePinnedToCore(receiver_task, "receiver_task", 4096, NULL, 20, NULL, 0);
}