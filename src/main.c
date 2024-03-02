#include "driver/adc.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "driver/touch_pad.h"
#include "driver/touch_sensor.h"
#include "driver/touch_sensor_common.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/touch_sensor_types.h"
#include "nvs_flash.h"
#include "remote/display.h"
#include "remote/peers.h"
#include "remote/receiver.h"
#include "remote/remote.h"
#include "remote/remoteinputs.h"
#include "remote/time.h"
#include "remote/transmitter.h"
#include "soc/touch_sensor_channel.h"
#include <hal/ledc_types.h>
#include <stdio.h>
#include <string.h>

#define TAG "PUBMOTE-MAIN"

uint8_t PEER_MAC_ADDRESS[6] = {72, 49, 183, 171, 63, 137};
int64_t LAST_COMMAND_TIME = 0;

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

  // Initialize ESP-NOW
  ESP_ERROR_CHECK(esp_now_init());
}

void IRAM_ATTR touch_isr() {
  // Add code to execute upon wake-up here
  ESP_LOGI(TAG, "Wake");
}

// TODO CRC checks on data

void app_main(void) {
  init_display();

  // ESPNOW init
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

  // ESPNOW callbacks
  ESP_ERROR_CHECK(esp_now_register_send_cb(on_data_sent));
  ESP_ERROR_CHECK(esp_now_register_recv_cb(on_data_recv));

  ESP_LOGI(TAG, "Registered RX callback. Creating tasks");
  xTaskCreate(send_data_task, "send_data_task", 2048, NULL, 2, NULL);

  // Remote inputs init
  register_button_isr();
  xTaskCreate(button_task, "button_task", 2048, NULL, 4, &buttonTaskHandle);
  xTaskCreate(throttle_task, "throttle_task", 2048, NULL, 3, NULL);

  // // Testing
  // touch_pad_config(TOUCH_PAD_NUM0, 500);
  // touch_pad_set_trigger_source(TOUCH_TRIGGER_SOURCE_SET1);
  // // Enable touch pad wake-up
  // esp_sleep_enable_touchpad_wakeup();

  // // Register ISR
  // touch_pad_isr_register(touch_isr, NULL);

  // vTaskDelay(10000 / portTICK_PERIOD_MS);
  // ESP_LOGI(TAG, "Entering sleep");
  // vTaskDelay(1000 / portTICK_PERIOD_MS);
  // esp_light_sleep_start();
}
