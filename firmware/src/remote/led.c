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
#include <math.h>
#include <nvs.h>

static const char *TAG = "PUBREMOTE-LED";

#if LED_ENABLED
  #define LEDC_CHANNEL LEDC_CHANNEL_2
  #define LEDC_TIMER LEDC_TIMER_2
  #define LED_COUNT 1
  #define LED_STRIP_RMT_RES_HZ (10 * 1000 * 1000)
static led_strip_handle_t led_strip;

static void configure_led(void) {
  // LED strip general initialization, according to your led board design
  led_strip_config_t strip_config = {
      .strip_gpio_num = LED_DATA_PIN,           // The GPIO that connected to the LED strip's data line
      .max_leds = LED_COUNT,                    // The number of LEDs in the strip,
      .led_pixel_format = LED_PIXEL_FORMAT_GRB, // Pixel format of your LED strip
      .led_model = LED_MODEL_WS2812,            // LED strip model
      .flags.invert_out = false,                // whether to invert the output signal
  };

  // LED strip backend configuration: RMT
  led_strip_rmt_config_t rmt_config = {
      .clk_src = RMT_CLK_SRC_DEFAULT,        // different clock source can lead to different power consumption
      .resolution_hz = LED_STRIP_RMT_RES_HZ, // RMT counter clock frequency
      .flags.with_dma = false,               // DMA feature is available on ESP target like ESP32-S3
  };

  // LED Strip object handle
  ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
  ESP_LOGI(TAG, "Created LED strip object with RMT backend");
}

static void led_power_on() {
  gpio_reset_pin(LED_POWER_PIN); // Initialize the pin
  gpio_set_direction(LED_POWER_PIN, GPIO_MODE_OUTPUT);
  gpio_set_level(LED_POWER_PIN, 1); // Turn on the LED power
}

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} RGB;

static RGB adjustBrightness(RGB originalColor, float brightnessScaling) {
  float gamma = 2.2; // Typical gamma value for WS2812 LEDs

  // Apply gamma correction to the scaling factor
  float correctedScaling = pow(brightnessScaling, gamma);
  RGB return_val;

  return_val.r = round(pow((originalColor.r / 255.0), gamma) * correctedScaling * 255.0);
  return_val.g = round(pow((originalColor.g / 255.0), gamma) * correctedScaling * 255.0);
  return_val.b = round(pow((originalColor.b / 255.0), gamma) * correctedScaling * 255.0);

  return return_val;
}

static void led_off() {
  ESP_ERROR_CHECK(led_strip_clear(led_strip));
  ESP_ERROR_CHECK(led_strip_refresh(led_strip));
}

static void led_task(void *pvParameters) {
  RGB base_color = {255, 105, 180};
  uint8_t brightness = 0;
  bool increasing = true;

  ESP_LOGD(TAG, "Start blinking LED strip");
  while (1) {
    RGB new_col = adjustBrightness(base_color, brightness / 255.0);

    for (int i = 0; i < LED_COUNT; i++) {
      ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, new_col.r, new_col.g, new_col.b));
    }
    /* Refresh the strip to send data */
    ESP_ERROR_CHECK(led_strip_refresh(led_strip));

    if (increasing) {
      brightness += 5;
    }
    else {
      brightness -= 5;
    }

    if (brightness >= 255) {
      increasing = false;
    }
    else if (brightness <= 0) {
      increasing = true;
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
  led_off();
  vTaskDelete(NULL);
}
#endif

void init_led() {
#if LED_ENABLED
  ESP_LOGI(TAG, "Initializing LED strip");
  led_power_on();
  configure_led();

  xTaskCreate(led_task, "led_task", 4096, NULL, 2, NULL);
#endif
}