#ifndef __GPIO_DETECTION_H
#define __GPIO_DETECTION_H
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef enum {
  EXTERNAL_PULL_NONE,
  EXTERNAL_PULL_UP,
  EXTERNAL_PULL_DOWN,
  EXTERNAL_PULL_ERROR
} external_pull_t;

external_pull_t detect_gpio_external_pull(int gpio_num);
bool gpio_supports_wakeup_from_deep_sleep(gpio_num_t gpio_num);

#endif