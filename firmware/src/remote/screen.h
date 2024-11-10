#include <core/lv_obj.h>
#ifndef __SCREEN_H
  #define __SCREEN_H

typedef struct {
  char *name;           // Name of the screen
  lv_obj_t *screen_obj; // Pointer to the screen's main container
  void (*create_ui)();  // Function to build the UI elements
  void (*on_show)();    // Optional function called when the screen is shown
  void (*on_hide)();    // Optional function called when the screen is hidden
  void *user_data;      // Pointer to user data

} RemoteScreen;

bool is_screen_visible(lv_obj_t *screen);

#endif