#ifndef __UTIL_H__
#define __UTIL_H__
#define DEBUG 0
#define SOCK_FD_MASK(long_fd) (0x00000000FFFFFFFF & (long long int)long_fd)

void handle_error(int);

#endif
