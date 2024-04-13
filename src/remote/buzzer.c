#include "esp_err.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "led_strip.h"
#include "nvs_flash.h"
#include <driver/ledc.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <led_strip_types.h>
#include <nvs.h>

static const char *TAG = "PUBMOTE-BUZZER";

#define BUZZER_GPIO GPIO_NUM_21

static void buzzer_on() {
  gpio_set_level(BUZZER_GPIO, 1); // Set GPIO high to turn on buzzer
}

static void buzzer_off() {
  gpio_set_level(BUZZER_GPIO, 0); // Set GPIO low to turn off buzzer
}

void init_buzzer() {
  gpio_reset_pin(BUZZER_GPIO); // Initialize the pin
  gpio_set_direction(BUZZER_GPIO, GPIO_MODE_OUTPUT);
  buzzer_on();
  vTaskDelay(1000 / portTICK_PERIOD_MS); // Wait for 1 second
  buzzer_off();
}