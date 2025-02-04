#ifndef __RECEIVER_H
#define __RECEIVER_H

#include <stdbool.h>
#include <stdio.h>

#define RSSI_NONE -100
#define RSSI_POOR -95
#define RSSI_FAIR -85
#define RSSI_GOOD -75

bool channel_lock();
void channel_unlock();
void init_receiver();

#endif