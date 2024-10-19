#ifndef __RECEIVER_H
#define __RECEIVER_H

#include <stdio.h>

void init_receiver();

typedef enum {
  PAIRING_STATE_UNPAIRED,
  PAIRING_STATE_PAIRING,
  PAIRING_STATE_PENDING,
  PAIRING_STATE_PAIRED
} PairingState;

extern PairingState pairing_state;
extern int32_t secret_code;
extern uint8_t remote_addr[6];
#endif