#include "screen.h"
#include <core/lv_disp.h>

#define MAX_SCREENS 5
static RemoteScreen registered_screens[MAX_SCREENS];
static int num_screens = 0;

// Function to register a screen
void router_register_screen(RemoteScreen *screen) {
  if (num_screens < MAX_SCREENS) {
    registered_screens[num_screens++] = *screen;
  }
  else {
    // Handle error: too many screens
  }
}

// Function to show a screen
void router_show_screen(const char *screen_name) {
  for (int i = 0; i < num_screens; i++) {
    if (strcmp(registered_screens[i].name, screen_name) == 0) {
      // Found the screen
      if (registered_screens[i].on_show) {
        registered_screens[i].on_show(&registered_screens[i]);
      }

      lv_disp_load_scr(registered_screens[i].screen_obj);
      // lv_obj_set_hidden(registered_screens[i].screen_obj, false);

      // Hide other screens
      // for (int j = 0; j < num_screens; j++) {
      //   if (j != i) {
      //     lv_obj_set_hidden(registered_screens[j].screen_obj, true);
      //   }
      // }
      return; // Screen shown
    }
  }
  // Handle error: Screen not found
}