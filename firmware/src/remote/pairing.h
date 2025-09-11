#ifndef __PAIRING_H
#define __PAIRING_H

#include "espnow.h"
#include <stdbool.h>
#include <stdio.h>

bool pairing_process_init_event(uint8_t *data, int len, esp_now_event_t evt);
bool pairing_process_bond_event(uint8_t *data, int len);
bool pairing_process_completion_event(uint8_t *data, int len);

#endif