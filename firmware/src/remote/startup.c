#include "startup.h"
#include "esp_log.h"

static const char *TAG = "PUBREMOTE-STARTUP";

static callback_registry_t startup_registry = {0};

void register_startup_cb(callback_t callback) {
  register_cb(&startup_registry, callback);
}

void remove_startup_cb(callback_t callback) {
  remove_cb(&startup_registry, callback);
}

void startup_cb() {
  registry_cb(&startup_registry, true); // Clean up after calling callbacks
}
