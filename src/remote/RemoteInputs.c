#include "RemoteInputs.h"
#include "Time.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rom/gpio.h"
#include <driver/adc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#define REMOTEINPUTS_TAG "PUBMOTE-REMOTEINPUTS"
// Configuration
#define BUTTON_PIN GPIO_NUM_0
#define LONG_PRESS_TIME 500
#define DOUBLE_PRESS_TIME 250

uint8_t THROTTLE_VALUE = 128;

// Global for Task Handle
TaskHandle_t buttonTaskHandle = NULL;

void throttle_task(void *pvParameters) {
  ESP_LOGI(REMOTEINPUTS_TAG, "Throttle task started");
  u_int8_t my_data[20] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19}; // Example data
#define THROTTLE_LOOP_TIME 10

  while (1) {
    // read potentiometer and apply to THROTTLE_VALUE
    adc1_config_width(ADC_WIDTH_BIT_DEFAULT); // ADC_WIDTH_BIT_12
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_0);
    int val = adc1_get_raw(ADC1_CHANNEL_0);
    THROTTLE_VALUE = val / 16;
    // ESP_LOGI(REMOTEINPUTS_TAG, "Throttle position: %d\n", THROTTLE_VALUE);
    vTaskDelay(THROTTLE_LOOP_TIME / portTICK_PERIOD_MS);
  }

  ESP_LOGI(REMOTEINPUTS_TAG, "Throttle task ended");
  // terminate self
  vTaskDelete(NULL);
}

static void IRAM_ATTR button_isr(void *arg) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE; // For task notification
  // Simple Debouncing Example:
  static uint64_t last_interrupt_time = 0;
  uint64_t current_time = get_current_time_ms();
  if (current_time - last_interrupt_time > 20) { // 20 ms debounce
    xTaskNotifyFromISR(buttonTaskHandle, 0, eNoAction, &xHigherPriorityTaskWoken);
    last_interrupt_time = current_time;
  }

  // Required for task scheduling from an ISR
  if (xHigherPriorityTaskWoken) {
    portYIELD_FROM_ISR();
  }
}

void register_button_isr() {
  gpio_config_t io_config = {.pin_bit_mask = (1ULL << BUTTON_PIN),
                             .mode = GPIO_MODE_INPUT,
                             .pull_up_en = GPIO_PULLUP_ENABLE,
                             .intr_type = GPIO_INTR_NEGEDGE};
  gpio_config(&io_config);

  // Install ISR Service and Add Handler
  gpio_install_isr_service(0);
  gpio_isr_handler_add(BUTTON_PIN, button_isr, NULL);
}

/// Your actions (replace placeholders)
static void shortPressAction() {
  // Code to execute for a short press
  printf("Short press detected!\n");
}

static void longPressAction() {
  // Code to execute for a long press
  printf("Long press detected!\n");
}

static void doublePressAction() {
  // Code to execute for a double press
  printf("Double press detected!\n");
}

// Button Handling Task
void button_task(void *params) {
  uint32_t notification;
  uint64_t lastPressTime = 0;

  while (true) {
    if (xTaskNotifyWait(0, 0, &notification, portMAX_DELAY)) {
      uint64_t pressDuration = get_current_time_ms() - lastPressTime;

      if (pressDuration < LONG_PRESS_TIME) {
        if (pressDuration < DOUBLE_PRESS_TIME) {
          doublePressAction();
        }
        else {
          shortPressAction();
        }
      }
      else {
        longPressAction();
      }
      lastPressTime = get_current_time_ms();
    }
  }
  ESP_LOGI(REMOTEINPUTS_TAG, "Button task ended");
  // terminate self
  vTaskDelete(NULL);
}