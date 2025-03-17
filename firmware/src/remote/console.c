
#include "console.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "PUBREMOTE-CONSOLE";

void init_console() {
  // Firmware version
#ifndef RELEASE_VARIANT
  #define RELEASE_VARIANT "dev"
#endif

  ESP_LOGI(TAG, "Version (%d.%d.%d.%s)", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, RELEASE_VARIANT);
  ESP_LOGI(TAG, "Variant (%s)", BUILD_TYPE);
}