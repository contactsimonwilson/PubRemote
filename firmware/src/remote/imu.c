#include "imu.h"
#include "buzzer.h"
#include "config.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "imu/imu_datatypes.h"
#include "imu/imu_driver.h"
#include "nvs_flash.h"
#include "settings.h"
#include <driver/gpio.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <nvs.h>

static const char *TAG = "PUBREMOTE-IMU";
static TaskHandle_t imu_task_handle = NULL;
static QueueHandle_t imu_event_queue = NULL;

#define DEBUG_IMU 0

static void IRAM_ATTR imu_isr_handler(void *arg) {
  uint32_t gpio_num = (uint32_t)arg;
  xQueueSendFromISR(imu_event_queue, &gpio_num, NULL);
}

static void imu_get_data() {
#if IMU_ENABLED
  imu_data_t imu_data = {0};
  imu_driver_get_data(&imu_data);

  #if DEBUG_IMU
  ESP_LOGI(TAG, "IMU Data - Accel: [%.2f, %.2f, %.2f], Gyro: [%.2f, %.2f, %.2f], Event: %d", imu_data.accel_x,
           imu_data.accel_y, imu_data.accel_z, imu_data.gyro_x, imu_data.gyro_y, imu_data.gyro_z, imu_data.event);
  #endif
#endif
}

void imu_task(void *pvParameters) {

#ifdef IMU_INT

#endif

  while (1) {
#ifdef IMU_INT
    uint32_t io_num;
    while (xQueueReceive(imu_event_queue, &io_num, 0) == pdTRUE) {
      imu_get_data();
    }
    vTaskDelay(pdMS_TO_TICKS(50));
#else
    imu_get_data();
    vTaskDelay(pdMS_TO_TICKS(50));
#endif
  }

  ESP_LOGI(TAG, "IMU task ended");
  // terminate self
  vTaskDelete(NULL);
}

void imu_init() {
#if IMU_ENABLED
  imu_driver_init();

  #ifdef IMU_INT
  gpio_config_t pmu_io_conf = {};
  pmu_io_conf.intr_type = GPIO_INTR_ANYEDGE;
  pmu_io_conf.mode = GPIO_MODE_INPUT;
  pmu_io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  pmu_io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  pmu_io_conf.pin_bit_mask = BIT64(PMU_INT);
  gpio_config(&pmu_io_conf);
  imu_event_queue = xQueueCreate(1, sizeof(uint32_t));

  gpio_isr_handler_add(PMU_INT, imu_isr_handler, (void *)IMU_INT);
  #endif

  xTaskCreate(imu_task, "imu_task", 4096, NULL, 2, &imu_task_handle);
#endif
}

void imu_deinit() {
#if IMU_ENABLED
  imu_driver_deinit();
  if (imu_task_handle != NULL) {
    vTaskDelete(imu_task_handle);
    imu_task_handle = NULL;
  }
  gpio_isr_handler_remove(IMU_INT);
#endif
}