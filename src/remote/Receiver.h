#ifndef __RECEIVER_H
#define __RECEIVER_H

#include <stdio.h>

#define RECEIVER_TAG "PUBMOTE_RECEIVER"

void on_data_recv(const uint8_t *mac_addr, const uint8_t *data, int len);
#endif