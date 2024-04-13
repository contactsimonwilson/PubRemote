#ifndef __RECEIVER_H
#define __RECEIVER_H

#include <stdio.h>

void init_receiver();
static void connection_timeout_callback(void *arg);
static void reconnecting_timer_callback(void *arg);
#endif