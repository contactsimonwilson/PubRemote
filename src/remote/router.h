#include "screen.h"
#ifndef ROUTER_H
  #define ROUTER_H

void router_register_screen(RemoteScreen *screen);
void router_show_screen(const char *screen_name);

#endif