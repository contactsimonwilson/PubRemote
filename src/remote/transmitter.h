#ifndef __TRANSMITTER_H
#define __TRANSMITTER_H
#include <stdio.h>

#include "esp_now.h"

#define TRANSMIT_TIME 20
#define TRANSMIT_FREQUENCY TRANSMIT_TIME / portTICK_PERIOD_MS

#define FIRMWARE_ID "PUBMOTE_0_0_1"

extern uint8_t PEER_MAC_ADDRESS[6]; // Replace XX with the actual MAC address bytes

typedef struct {
  char *firmwareId;
} ParingInfo;

typedef struct {
  u_int16_t size;
  uint16_t *results;
} LatencyTestResults;

void init_transmitter();

#endif