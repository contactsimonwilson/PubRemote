#ifndef __RECEIVER_H
#define __RECEIVER_H

#include <stdbool.h>
#include <stdio.h>

bool channel_lock();
void channel_unlock();
void init_receiver();

#endif