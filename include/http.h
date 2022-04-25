#ifndef __HTTPS_H__
#define __HTTPS_H__
#include <pthread.h>
#define SERVER_NAME "CServer"

enum http_status {
	HTTP_CONTINUE = 100,
	HTTP_OK = 200,
	HTTP_NO_CONTENT = 204,
	HTTP_MOVED_PERMANENTLY = 301,
	HTTP_FOUND_MOVED_TEMPORARILY = 302,
	HTTP_NOT_MODIFIED = 304,
	HTTP_BAD_REQUEST = 400,
	HTTP_UNAUTHORIZED = 401,
	HTTP_FORBIDDEN = 403,
	HTTP_NOT_FOUND = 404,
	HTTP_INTERNAL_SERVER_ERROR = 500,
	HTTP_NOT_IMPLEMENTED = 501,
	HTTP_BAD_GATEWAY = 502
};

enum http_request_method {
	HTTP_UNKNOWN = 0x0,
	HTTP_GET = 0x1,
	HTTP_POST = 0x2,
	HTTP_HEAD = 0x4
};

struct http_status_table {
	uint16_t status_code;
	const char *status_name;
};

int init_http_settings(int, char **);

void http_request(void *);

#endif
