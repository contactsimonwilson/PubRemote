#ifndef __COMMANDS_H
#define __COMMANDS_H

#include <stdbool.h>
#include <stdio.h>

typedef enum {
  REM_VERSION = 0,
  REM_RECEIVER_VERSION = 1,
  REM_PAIR_INIT = 10,
  REM_PAIR_BOND = 11,
  REM_PAIR_COMPLETE = 12,
  REM_SET_CORE_DATA = 100,
} RemoteCommands;

typedef enum {
  REC_VERSION = 0,
  REC_SET_REMOTE_STATE = 100,
} ReceiverCommands;

bool process_board_data(uint8_t *data, int len);

#endif