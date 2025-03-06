#ifndef __OTA_H
#define __OTA_H

#include "esp_flash_partitions.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#define FIRMWARE_BUFFER_SIZE 1024
#define OTA_TIMEOUT_MS 60000 // 60 seconds timeout

typedef struct {
  bool in_progress;
  int total_size;
  int received_size;
  int progress;
  esp_ota_handle_t ota_handle;
  const esp_partition_t *update_partition;
} ota_state_t;

void init_ota();
void teardown_ota();
int ota_get_progress(void);
bool ota_is_updating(void);

#endif