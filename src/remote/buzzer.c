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

static const char *TAG = "PUBREMOTE-BUZZER";

#define BUZZER_PIN 21
#define BUZZER_CHANNEL LEDC_CHANNEL_1
#define BUZZER_TIMER LEDC_TIMER_1
#define BUZZER_FREQUENCY 440 // Frequency in Hz (e.g., 440Hz for A4 note)

void buzzer_on() {
  // Start the LEDC output
  ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, 512); // 50% duty cycle
  ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL);
}

// Function to turn the buzzer OFF
void buzzer_off() {
  // Stop the LEDC output (effectively turning off the buzzer)
  ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, 0);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL);
}

void init_buzzer() {
  // Configure LEDC (PWM)
  ledc_timer_config_t timer_conf = {.speed_mode = LEDC_LOW_SPEED_MODE,
                                    .timer_num = BUZZER_TIMER,
                                    .duty_resolution = LEDC_TIMER_10_BIT, // Resolution of PWM duty
                                    .freq_hz = BUZZER_FREQUENCY,          // Frequency of the signal
                                    .clk_cfg = LEDC_AUTO_CLK};
  ledc_timer_config(&timer_conf);

  ledc_channel_config_t channel_conf = {.gpio_num = BUZZER_PIN,
                                        .speed_mode = LEDC_LOW_SPEED_MODE,
                                        .channel = BUZZER_CHANNEL,
                                        .intr_type = LEDC_INTR_DISABLE,
                                        .timer_sel = BUZZER_TIMER,
                                        .duty = 0, // Initially off
                                        .hpoint = 0};
  ledc_channel_config(&channel_conf);

  buzzer_on();
  vTaskDelay(pdMS_TO_TICKS(1000)); // Wait for 1 second
  buzzer_off();
}