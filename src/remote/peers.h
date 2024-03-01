#ifndef __PEERS_H
#define __PEERS_H
#include <stdio.h>

extern int64_t LAST_COMMAND_TIME;

typedef struct {
  uint8_t mac[6];   // MAC address storage
  char name[32];    // Device name (adjust size as needed)
  u_int8_t channel; // Device channel
} SavedPeer;

typedef struct {
  u_int8_t deviceCount;
  SavedPeer *devices;
} SavedPeers;
#endif