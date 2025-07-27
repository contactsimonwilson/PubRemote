#include "led.h"
#include "config.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "led_strip.h"
#include "nvs_flash.h"
#include "settings.h"
#include "stats.h"
#include "time.h"
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
  #define LED_STRIP_RMT_RES_HZ (10 * 1000 * 1000)
static led_strip_handle_t led_strip = NULL;

static uint8_t brightness_level = 255; // Brightness setting for the LED strip
static uint8_t current_brightness = 0; // Effective brightness for the animation
static RGB rgb = {255, 255, 255};
static LedEffect current_effect = LED_EFFECT_NONE;

  #define ANIMATION_DELAY_MS 10 // Delay for the LED animation task
  #define BRIGHTNESS_STEP 5     // Step size for brightness adjustment

static void configure_led(void) {
  // LED strip general initialization, according to your led board design
  led_strip_config_t strip_config = {
      .strip_gpio_num = LED_DATA,               // The GPIO that connected to the LED strip's data line
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
  #ifdef LED_POWER_PIN
  gpio_reset_pin(LED_POWER_PIN); // Initialize the pin
  gpio_set_direction(LED_POWER_PIN, GPIO_MODE_OUTPUT);
  gpio_set_level(LED_POWER_PIN, 1); // Turn on the LED power
  #endif
}

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

static void apply_led_effect() {
  if (led_strip == NULL) {
    ESP_LOGE(TAG, "LED strip not initialized");
    return;
  }

  RGB new_col = adjustBrightness(rgb, current_brightness / 255.0);

  for (int i = 0; i < LED_COUNT; i++) {
    ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, new_col.r, new_col.g, new_col.b));
  }
  /* Refresh the strip to send data */
  ESP_ERROR_CHECK(led_strip_refresh(led_strip));
}

static RGB hex_to_rgb(uint32_t hex) {
  RGB color;
  color.r = (hex >> 16) & 0xFF;
  color.g = (hex >> 8) & 0xFF;
  color.b = hex & 0xFF;
  return color;
}

static void pulse_effect() {
  static bool increasing = true;

  if (increasing) {
    current_brightness += BRIGHTNESS_STEP;
  }
  else {
    current_brightness -= BRIGHTNESS_STEP;
  }

  if (current_brightness >= brightness_level) {
    current_brightness = brightness_level;
    increasing = false;
  }
  else if (current_brightness <= 0) {
    current_brightness = 0;
    increasing = true;
  }
  apply_led_effect();
  vTaskDelay(pdMS_TO_TICKS(ANIMATION_DELAY_MS));
}

static void solid_effect() {
  current_brightness = brightness_level;
  apply_led_effect();
  vTaskDelay(pdMS_TO_TICKS(ANIMATION_DELAY_MS));
}

static void no_effect() {
  current_brightness = 0;
  apply_led_effect();
  vTaskDelay(pdMS_TO_TICKS(ANIMATION_DELAY_MS));
}

void set_led_effect_solid(uint32_t color) {
  rgb = hex_to_rgb(color);
  current_effect = LED_EFFECT_SOLID;
  current_brightness = brightness_level;
  apply_led_effect();
  // TODO - Spawn task (kill existing task if running)
}

void set_led_effect_pulse(uint32_t color) {
  rgb = hex_to_rgb(color);
  current_effect = LED_EFFECT_PULSE;
  current_brightness = brightness_level;
  apply_led_effect();
  // TODO - Spawn task (kill existing task if running)
}

void set_led_effect_none() {
  current_effect = LED_EFFECT_NONE;
  current_brightness = 0;
  apply_led_effect();
  // TODO - Kill running task
}

// TODO - make this a spawned task rather than always running
static void led_task(void *pvParameters) {
  bool is_booting = true;
  set_led_effect_pulse(device_settings.theme_color);
  apply_led_effect();

  while (1) {
    if (is_booting) {
      // Set is_booting to false after 3 seconds
      is_booting = get_current_time_ms() < 3000;
      if (!is_booting) {
        current_effect = LED_EFFECT_NONE;
      }
    }

    if (current_effect == LED_EFFECT_PULSE) {
      pulse_effect();
      continue;
    }

    if (current_effect == LED_EFFECT_SOLID) {
      solid_effect();
      continue;
    }

    if (current_effect == LED_EFFECT_NONE) {
      no_effect();
      continue;
    }

    // Default off in case of an unknown effect
    ESP_LOGW(TAG, "Unknown LED effect: %d", current_effect);
    current_brightness = 0;
    apply_led_effect();
    vTaskDelay(pdMS_TO_TICKS(ANIMATION_DELAY_MS));
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

void set_led_brightness(uint8_t brightness) {
#if LED_ENABLED
  brightness_level = brightness;
  apply_led_effect();
#endif
}