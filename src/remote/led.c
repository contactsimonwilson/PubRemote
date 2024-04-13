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

static const char *TAG = "PUBMOTE-LED";

#define LED_POWER_GPIO GPIO_NUM_33
#define LED_GPIO GPIO_NUM_18
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_TIMER LEDC_TIMER_0

// Numbers of the LED in the strip
#define LED_STRIP_LED_NUMBERS 1
// 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define LED_STRIP_RMT_RES_HZ (10 * 1000 * 1000)

led_strip_handle_t configure_led(void) {
  // LED strip general initialization, according to your led board design
  led_strip_config_t strip_config = {
      .strip_gpio_num = LED_GPIO,                // The GPIO that connected to the LED strip's data line
      .max_leds = LED_STRIP_LED_NUMBERS,         // The number of LEDs in the strip,
      .led_pixel_format = LED_PIXEL_FORMAT_GRBW, // Pixel format of your LED strip
      .led_model = LED_MODEL_WS2812,             // LED strip model
      .flags.invert_out = false,                 // whether to invert the output signal
  };

  // LED strip backend configuration: RMT
  led_strip_rmt_config_t rmt_config = {
      .clk_src = RMT_CLK_SRC_DEFAULT,        // different clock source can lead to different power consumption
      .resolution_hz = LED_STRIP_RMT_RES_HZ, // RMT counter clock frequency
      .flags.with_dma = false,               // DMA feature is available on ESP target like ESP32-S3
  };

  // LED Strip object handle
  led_strip_handle_t led_strip;
  ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
  ESP_LOGI(TAG, "Created LED strip object with RMT backend");
  return led_strip;
}

void led_power_on() {
  gpio_reset_pin(LED_POWER_GPIO); // Initialize the pin
  gpio_set_direction(LED_POWER_GPIO, GPIO_MODE_OUTPUT);
}

void init_led() {
  led_power_on();
  led_strip_handle_t led_strip = configure_led();
  // bool led_on_off = false;

  // // LEDC timer configuration
  // ledc_timer_config_t ledc_timer = {
  //     .speed_mode = LEDC_LOW_SPEED_MODE,
  //     .duty_resolution = LEDC_TIMER_13_BIT,
  //     .timer_num = LEDC_TIMER,
  //     .freq_hz = 200,
  // };
  // ledc_timer_config(&ledc_timer);

  // // LEDC channel configuration
  // ledc_channel_config_t ledc_channel = {
  //     .gpio_num = LED_GPIO,
  //     .speed_mode = LEDC_LOW_SPEED_MODE,
  //     .channel = LEDC_CHANNEL,
  //     .intr_type = LEDC_INTR_DISABLE,
  //     .timer_sel = LEDC_TIMER,
  //     .duty = 4096 // Approximately 50% duty cycle
  // };
  // ledc_channel_config(&ledc_channel);

  // ESP_LOGI(TAG, "Start blinking LED strip");

  // while (1) {
  //   if (led_on_off) {
  //     /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
  //     for (int i = 0; i < LED_STRIP_LED_NUMBERS; i++) {
  //       ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, 255, 255, 255));
  //     }
  //     /* Refresh the strip to send data */
  //     ESP_ERROR_CHECK(led_strip_refresh(led_strip));
  //     ESP_LOGI(TAG, "LED ON!");
  //   }
  //   else {
  //     /* Set all LED off to clear all pixels */
  //     ESP_ERROR_CHECK(led_strip_clear(led_strip));
  //     ESP_LOGI(TAG, "LED OFF!");
  //   }

  //   led_on_off = !led_on_off;
  //   vTaskDelay(pdMS_TO_TICKS(500));
  // }
}