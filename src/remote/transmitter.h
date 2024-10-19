#ifndef __TRANSMITTER_H
#define __TRANSMITTER_H
#include <stdint.h>
#include <stdio.h>

#include "esp_now.h"

#define FIRMWARE_ID "PUBREMOTE-0_0_1"

typedef struct {
  char *firmwareId;
} ParingInfo;

typedef struct {
  u_int16_t size;
  uint16_t *results;
} LatencyTestResults;

void init_transmitter();

#endif