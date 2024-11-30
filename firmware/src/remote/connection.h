#ifndef __CONNECTION_H
#define __CONNECTION_H
#include <esp_timer.h>

typedef enum {
  CONNECTION_STATE_DISCONNECTED,
  CONNECTION_STATE_CONNECTING,
  CONNECTION_STATE_CONNECTED,
  CONNECTION_STATE_RECONNECTING
} ConnectionState;

typedef enum {
  PAIRING_STATE_UNPAIRED,
  PAIRING_STATE_PAIRING,
  PAIRING_STATE_PENDING,
  PAIRING_STATE_PAIRED
} PairingState;

extern ConnectionState connection_state;
extern PairingState pairing_state;

void update_connection_state(ConnectionState state);
void init_connection();
void connect_to_peer(uint8_t *mac_addr, uint8_t channel);
void connect_to_default_peer();

#endif