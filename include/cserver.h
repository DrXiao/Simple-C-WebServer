#ifndef __C_SERVER_H__
#define __C_SERVER_H__
#include <stdint.h>
#define LISTEN_NUMS 1024
#define THREAD 32
#define QUEUE 1024

int init_server(const char *, uint16_t, int);

#endif
