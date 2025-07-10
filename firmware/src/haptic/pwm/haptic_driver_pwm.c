#include "haptic_driver_pwm.h"
#include "config.h"
#include "esp_err.h"
#include "esp_log.h"
#include <driver/ledc.h>

static const char *TAG = "PUBREMOTE-HAPTIC_DRIVER_PWM";

#if HAPTIC_ENABLED
  #define HAPTIC_CHANNEL LEDC_CHANNEL_3
  #define HAPTIC_TIMER LEDC_TIMER_3
  #define HAPTIC_RESOLUTION LEDC_TIMER_10_BIT
  #define HAPTIC_MAX_DUTY ((1 << 10) - 1)
#endif

esp_err_t pwm_haptic_driver_init() {
#if HAPTIC_ENABLED && defined(HAPTIC_PWM)
  #error "Implementation for HAPTIC_PWM is not complete yet. Please implement the PWM haptic driver."
  // Initialize the PWM for haptic feedback
  ledc_timer_config_t timer_conf = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .timer_num = HAPTIC_TIMER,
      .duty_resolution = HAPTIC_RESOLUTION,
      .freq_hz = 1000,
      .clk_cfg = LEDC_AUTO_CLK,
  };
  ledc_timer_config(&timer_conf);

  ledc_channel_config_t channel_conf = {
      .gpio_num = HAPTIC_PWM,
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = HAPTIC_CHANNEL,
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = HAPTIC_TIMER,
      .duty = 0, // Initially off
      .hpoint = 0,
  };
  ledc_channel_config(&channel_conf);
  // Set the initial duty cycle to 0 (off)
  ledc_set_duty(LEDC_LOW_SPEED_MODE, HAPTIC_CHANNEL, 0);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, HAPTIC_CHANNEL);

#else
  ESP_LOGW(TAG, "Haptic driver not enabled. Please check your configuration.");
#endif

  ESP_LOGI(TAG, "PWM haptic driver initialized successfully.");
  return ESP_OK;
}