#include "startup.h"
#include "esp_log.h"

static const char *TAG = "PUBREMOTE-STARTUP";

static callback_registry_t registry = {0};

static void cleanup_registry() {
  free(registry.callbacks);
  registry.callbacks = NULL;
  registry.count = registry.capacity = 0;
}

void register_startup_cb(callback_t callback) {
  if (registry.count >= registry.capacity) {
    size_t new_capacity = registry.capacity + 1; // Just add 1
    callback_t *new_callbacks = realloc(registry.callbacks, new_capacity * sizeof(callback_t));
    if (!new_callbacks) {
      ESP_LOGE(TAG, "Failed to allocate memory for callback registry");
      return;
    }
    registry.callbacks = new_callbacks;
    registry.capacity = new_capacity;
  }

  registry.callbacks[registry.count++] = callback;
}

void remove_startup_cb(callback_t callback) {
  for (size_t i = 0; i < registry.count; i++) {
    if (registry.callbacks[i] == callback) {
      // Shift remaining callbacks down
      for (size_t j = i; j < registry.count - 1; j++) {
        registry.callbacks[j] = registry.callbacks[j + 1];
      }
      registry.count--;
      return;
    }
  }
  ESP_LOGW(TAG, "Callback not found in registry");
}

void startup_cb() {
  for (size_t i = 0; i < registry.count; i++) {
    registry.callbacks[i]();
  }
  cleanup_registry();
}
