#ifndef __STARTUP_H
#define __STARTUP_H
#include <stdio.h>
#include <stdlib.h>

typedef void (*callback_t)(void);

typedef struct {
  callback_t *callbacks;
  size_t count;
  size_t capacity;
} callback_registry_t;

void register_startup_cb(callback_t callback);
void remove_startup_cb(callback_t callback);
void startup_cb();

#endif
