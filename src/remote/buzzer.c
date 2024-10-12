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
#define BUZZER_RESOLUTION LEDC_TIMER_10_BIT
#define MAX_DUTY ((1 << 10) - 1)

// Define notes (frequencies in Hz)
#define NOTE_C4 261
#define NOTE_D4 294
#define NOTE_E4 329
#define NOTE_F4 349
#define NOTE_G4 392
#define NOTE_A4 440
#define NOTE_B4 493
#define NOTE_C5 523

// mutex for buzzer
static SemaphoreHandle_t buzzer_mutex;

// Note (hz), volume (0-100), duration (ms)
int melody[] = {NOTE_C4, 100, 100, NOTE_D4, 100, 100, NOTE_E4, 100, 100, NOTE_F4, 100, 100,
                NOTE_G4, 100, 100, NOTE_A4, 100, 100, NOTE_B4, 100, 100, NOTE_C5, 100, 200};

int notes = sizeof(melody) / sizeof(melody[0]) / 3; // Number of notes

void play_note(int frequency, int volume, int duration) {
  // Take the mutex
  if (buzzer_mutex == NULL) {
    buzzer_mutex = xSemaphoreCreateMutex();
  }
  xSemaphoreTake(buzzer_mutex, portMAX_DELAY);
  // Configure the timer with the new frequency
  ledc_timer_config_t timer_conf = {.speed_mode = LEDC_LOW_SPEED_MODE,
                                    .timer_num = BUZZER_TIMER,
                                    .duty_resolution = BUZZER_RESOLUTION,
                                    .freq_hz = frequency,
                                    .clk_cfg = LEDC_AUTO_CLK};
  ledc_timer_config(&timer_conf);

  // Calculate duty cycle based on volume (0-100)
  uint8_t final_volume = volume > 100 ? 100 : volume;
  final_volume = volume < 0 ? 0 : volume;
  uint32_t duty = MAX_DUTY - (((float)final_volume / 100) * 100); // Level is inverted
  ESP_LOGD(TAG, "Playing note at %d Hz with volume %d (duty: %ld) and duration %d ms", frequency, final_volume, duty,
           duration);

  // Start the buzzer
  ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, duty);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL);

  vTaskDelay(duration / portTICK_PERIOD_MS);

  // Stop the buzzer
  ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, MAX_DUTY);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL);

  // Release the mutex
  xSemaphoreGive(buzzer_mutex);
}

// task to play melody
void play_melody_task(void *pvParameters) {
  for (int i = 0; i < notes; i++) {
    play_note(melody[i * 3], melody[i * 3 + 1], melody[i * 3 + 2]);
  }
  vTaskDelete(NULL);
}

void play_melody() {
  xTaskCreate(play_melody_task, "play_melody_task", 4096, NULL, 2, NULL);
}

void init_buzzer() {
  ledc_channel_config_t channel_conf = {.gpio_num = BUZZER_PIN,
                                        .speed_mode = LEDC_LOW_SPEED_MODE,
                                        .channel = BUZZER_CHANNEL,
                                        .intr_type = LEDC_INTR_DISABLE,
                                        .timer_sel = BUZZER_TIMER,
                                        .duty = 0, // Initially off
                                        .hpoint = 0};
  ledc_channel_config(&channel_conf);

  play_melody();
}