#ifndef __HTTPS_H__
#define __HTTPS_H__
#include <pthread.h>
#define SERVER_NAME "CServer"


enum http_status {
    HTTP_OK = 200,
    HTTP_NOT_MODIFIED = 304,
    HTTP_NOT_FOUND = 404
};

void http_request(void *);

void http_parse(const char *);

void http_reply(int);

#endif