#ifndef __PAIRING_H
#define __PAIRING_H

#include "espnow.h"
#include <stdbool.h>
#include <stdio.h>

bool process_pairing_init(uint8_t *data, int len, esp_now_event_t evt);
bool process_pairing_bond(uint8_t *data, int len);
bool process_pairing_complete(uint8_t *data, int len);

#endif