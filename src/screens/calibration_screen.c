#include "esp_log.h"
#include "remote/screen.h"
#include <remote/display.h>
#include <remote/remoteinputs.h>
#include <remote/settings.h>
#include <ui/ui.h>

static const char *TAG = "PUBREMOTE-CALIBRATION_SCREEN";

bool is_calibration_screen_active() {
  lv_obj_t *active_screen = lv_scr_act();
  return active_screen == ui_CalibrationScreen;
}

void calibration_task(void *pvParameters) {
  while (is_calibration_screen_active()) {
    char *formattedString;
    asprintf(&formattedString, "X: %.1f (%d)\nY: %.1f (%d)", remote_data.data.js_x, joystick_data.x,
             remote_data.data.js_y, joystick_data.y);
    lv_label_set_text(ui_CalibrationDataLabel, formattedString);
    vTaskDelay(pdMS_TO_TICKS(100));
  }

  ESP_LOGI(TAG, "Calibration task ended");
  vTaskDelete(NULL);
}

// Event handlers
void calibration_screen_loaded(lv_event_t *e) {
  ESP_LOGI(TAG, "Calibration screen loaded");

  // start task to update UI
  xTaskCreate(calibration_task, "calibration_task", 4096, NULL, 2, NULL);
}

void calibration_screen_unloaded(lv_event_t *e) {
  ESP_LOGI(TAG, "Calibration screen unloaded");
}

// Event handlers
void calibration_settings_save(lv_event_t *e) {
  // Your code here
}