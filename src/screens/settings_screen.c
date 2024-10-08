#include <remote/display.h>
#include <remote/powermanagement.h>
#include <remote/settings.h>
#include <ui/ui.h>

static const char *TAG = "PUBREMOTE-SETTINGS_SCREEN";

// Event handlers
void enter_deep_sleep(lv_event_t *e) {
  enter_sleep();
}