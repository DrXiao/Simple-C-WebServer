#ifndef __HTTPS_H__
#define __HTTPS_H__
#include <pthread.h>
#define SERVER_NAME "CServer"

void init_http_settings(int, char **);

void http_request(void *);

#endif
