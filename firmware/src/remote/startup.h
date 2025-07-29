#ifndef __STARTUP_H
#define __STARTUP_H
#include "utilities/callback_registry.h"

void register_startup_cb(callback_t callback);
void remove_startup_cb(callback_t callback);
void startup_cb();

#endif
