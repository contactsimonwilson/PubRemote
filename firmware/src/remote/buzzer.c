#include "buzzer.h"
#include "config.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "led_strip.h"
#include "nvs_flash.h"
#include "settings.h"
#include "tones.h"
#include <driver/ledc.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <led_strip_types.h>
#include <nvs.h>

static const char *TAG = "PUBREMOTE-BUZZER";

#if BUZZER_ENABLED
  #define BUZZER_CHANNEL LEDC_CHANNEL_1
  #define BUZZER_TIMER LEDC_TIMER_1
  #define BUZZER_RESOLUTION LEDC_TIMER_10_BIT
  #define BUZZER_MAX_DUTY ((1 << 10) - 1)
#endif

#if BUZZER_ENABLED
// mutex for buzzer
static SemaphoreHandle_t buzzer_mutex;

// Note (hz), duration (ms)
static const int melody[] = {NOTE_C4, 100, NOTE_D4, 100, NOTE_E4, 100, NOTE_F4, 100,
                             NOTE_G4, 100, NOTE_A4, 100, NOTE_B4, 100, NOTE_C5, 200};

static const int notes = sizeof(melody) / sizeof(melody[0]) / 2; // Number of notes
#endif

void play_note(int frequency, int duration) {
#if BUZZER_ENABLED
  // Take the mutex
  xSemaphoreTake(buzzer_mutex, portMAX_DELAY);
  // Configure the timer with the new frequency
  ledc_timer_config_t timer_conf = {.speed_mode = LEDC_LOW_SPEED_MODE,
                                    .timer_num = BUZZER_TIMER,
                                    .duty_resolution = BUZZER_RESOLUTION,
                                    .freq_hz = frequency,
                                    .clk_cfg = LEDC_AUTO_CLK};
  ledc_timer_config(&timer_conf);

  // Calculate duty cycle based on volume (0-100)
  uint32_t duty = BUZZER_MAX_DUTY / 2;
  ESP_LOGD(TAG, "Playing note at %d Hz with duration %d ms", frequency, duration);

  // Start the buzzer
  ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, duty);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL);

  vTaskDelay(duration / portTICK_PERIOD_MS);

  // Stop the buzzer
  #if BUZZER_INVERT
  ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, BUZZER_MAX_DUTY);
  #else
  ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, 0);
  #endif
  ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL);

  // Release the mutex
  xSemaphoreGive(buzzer_mutex);
#endif
}

#if BUZZER_ENABLED
// task to play melody
static void play_melody_task(void *pvParameters) {
  for (int i = 0; i < notes; i++) {
    play_note(melody[i * 2], melody[i * 2 + 1]);
  }
  vTaskDelete(NULL);
}
#endif

void play_melody() {
#if BUZZER_ENABLED
  xTaskCreate(play_melody_task, "play_melody_task", 4096, NULL, 2, NULL);
#endif
}

void init_buzzer() {
#if BUZZER_ENABLED
  if (buzzer_mutex == NULL) {
    buzzer_mutex = xSemaphoreCreateMutex();
  }

  ledc_channel_config_t channel_conf = {
      .gpio_num = BUZZER_PWM,
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = BUZZER_CHANNEL,
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = BUZZER_TIMER,
  #if BUZZER_INVERT
      .duty = BUZZER_MAX_DUTY,
  #else
      .duty = 0, // Initially off
  #endif
      .hpoint = 0,
  };
  ledc_channel_config(&channel_conf);
#endif
}

void play_startup_sound() {
#if BUZZER_ENABLED
  ESP_LOGI(TAG, "Playing startup sound");
  if (device_settings.startup_sound == STARTUP_SOUND_MELODY) {
    play_melody();
  }
  else if (device_settings.startup_sound == STARTUP_SOUND_BEEP) {
    play_note(NOTE_C5, 300);
  }
#endif
}