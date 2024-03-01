#ifndef __REMOTE_H
#define __REMOTE_H

#include <stdio.h>

typedef enum {
  BOOT,
  DISCONNECTED,
  PAIRING,
  CONNECTING,
  RECONNECTING,
  CONNECTED,
} RemoteState;

typedef enum {
  COMM_PAIR,
  COMM_CONNECT,
  COMM_DISCONNECT,
  COMM_TEST,
  COMM_RTDATA,
  COMM_REMOTE_CONTROL,
} RemoteCommand;

typedef struct {
  RemoteCommand command;
  u_int8_t *payload;
} RemotePayload;

typedef enum {
  REMOTE_TASK_NONE,
  REMOTE_TASK_PAIR,
  REMOTE_TASK_TEST,
  REMOTE_TASK_OPERATE,
} RemoteTask;

#endif