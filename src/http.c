#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "http.h"
#include "util.h"
#define MAXLEN 8192 + 4
#define MAX_FILE_NAME_LEN 64
#define HTTP_VER "HTTP/1.1" /* HTTP Start Line */

#if DEBUG == 1
#include <pthread.h>
#endif

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

struct http_request_msg {
	int client_sockfd;
	char recv_msg[MAXLEN];
	char *request_line, *header, *body;
	// Request line
	int method;
	const char *http_ver;
	// Request header
};

struct http_response_msg {
	int client_sockfd;
	struct http_status_table *status;
	char filename[MAX_FILE_NAME_LEN];
	struct stat file_stat;
	char file_mtime[36];
	struct mime_type *mime;
	char header[MAXLEN];
	char body[MAXLEN];
};

struct mime_type {
	const char *type;
	const char *value;
};

static char *file_dir = "www";
static char *homepage = "index.html";

static struct http_status_table http_status_table[] = {
	{HTTP_OK, "OK"},
	{HTTP_NOT_MODIFIED, "Not Modified"},
	{HTTP_NOT_FOUND, "Not Found"},
};

static struct mime_type mime_type[] = {{".html", "text/html"},
				       {".json", "application/json"},
				       {NULL, "text/plain"}};

static void http_parse(struct http_request_msg *, struct http_response_msg *);

static void http_parse_request_line(struct http_request_msg *,
				    struct http_response_msg *);

static void http_parse_request_header(struct http_request_msg *,
				      struct http_response_msg *);

static void http_parse_request_body(struct http_request_msg *,
				    struct http_response_msg *);

static void http_reply(struct http_response_msg *);

static void http_reply_err(struct http_response_msg *);

#if DEBUG == 1
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static int requests = 0;
#endif

void parse_arg(char *arg) {
}

void init_http_settings(int argc, char **argv) {
}

void http_request(void *sockfd) {
#if DEBUG == 1
	pthread_mutex_lock(&lock);
	requests++;
	pthread_mutex_unlock(&lock);
#endif

	struct http_request_msg http_req_msg;
	struct http_response_msg http_res_msg;
	memset(&http_req_msg, 0, sizeof(http_req_msg));
	memset(&http_res_msg, 0, sizeof(http_res_msg));
	http_req_msg.client_sockfd = http_res_msg.client_sockfd = (int)sockfd;
	recv(http_req_msg.client_sockfd, http_req_msg.recv_msg,
	     sizeof(http_req_msg.recv_msg), 0);

	if (!strcmp(http_req_msg.recv_msg, "")) {
		printf("sockfd(%d) No any message\n",
		       http_req_msg.client_sockfd);
		goto close;
	}
	http_parse(&http_req_msg, &http_res_msg);
	http_reply(&http_res_msg);

close:
	shutdown(http_req_msg.client_sockfd, SHUT_RDWR);
	close(http_req_msg.client_sockfd);
}

static void http_parse(struct http_request_msg *http_req_msg,
		       struct http_response_msg *http_res_msg) {
	char *parser = http_req_msg->recv_msg;

	http_req_msg->request_line = parser;

	parser = strstr(http_req_msg->recv_msg, "\r\n");
	http_req_msg->header = parser + 2;

	parser = strstr(http_req_msg->recv_msg, "\r\n\r\n");
	http_req_msg->body = parser + 4;

	http_parse_request_line(http_req_msg, http_res_msg);
	http_parse_request_header(http_req_msg, http_res_msg);
	http_parse_request_body(http_req_msg, http_res_msg);
}

static void http_parse_request_line(struct http_request_msg *http_req_msg,
				    struct http_response_msg *http_res_msg) {
	char *method = NULL, *uri = NULL, *http_ver = NULL, *parser = NULL;

	parser = http_req_msg->request_line;
	method = parser;

	while (*parser != '\r' && *parser != '\n') {
		if (*parser == ' ') {
			if (!uri)
				uri = parser + 1;
			else if (!http_ver) {
				http_ver = parser + 1;
			}
		}
		parser++;
	}

	if (!strncmp(method, "GET", 3))
		http_req_msg->method = HTTP_GET;
	else if (!strncmp(method, "POST", 4))
		http_req_msg->method = HTTP_POST;
	else if (!strncmp(method, "HEAD", 4))
		http_req_msg->method = HTTP_HEAD;

	strcat(http_res_msg->filename, file_dir);
	strncat(http_res_msg->filename, uri, (size_t)(http_ver - uri) - 1);

	if (!strncmp(uri, "/", (size_t)(http_ver - uri) - 1))
		strcat(http_res_msg->filename, homepage);

	int mime_idx = 0;
	for (; mime_idx < sizeof(mime_type) / sizeof(struct mime_type) - 1;
	     mime_idx++) {
		if (strstr(http_res_msg->filename, mime_type[mime_idx].type)) {
			break;
		}
	}
	http_res_msg->mime = mime_type + mime_idx;

	if (!strncmp(http_ver, HTTP_VER, sizeof(HTTP_VER)))
		http_req_msg->http_ver = HTTP_VER;
}

static void http_parse_request_header(struct http_request_msg *http_req_msg,
				      struct http_response_msg *http_res_msg) {
	printf("%s\n", http_req_msg->header);
}

static void http_parse_request_body(struct http_request_msg *http_req_msg,
				    struct http_response_msg *http_res_msg) {
	if (http_req_msg->method == HTTP_POST) {
		printf("%s\n", http_req_msg->body);
	}
}

static void http_reply(struct http_response_msg *http_res_msg) {

	if (stat(http_res_msg->filename, &http_res_msg->file_stat) < 0) {
		printf("File: (%s) Not Found\n", http_res_msg->filename);
		http_res_msg->status = http_status_table + 2;
		http_res_msg->mime = mime_type;
		http_reply_err(http_res_msg);
		goto reply_end;
	}

	http_res_msg->status = http_status_table;

	strftime(http_res_msg->file_mtime, sizeof(http_res_msg->file_mtime),
		 "%a, %d %b %Y %H:%M:%S GMT",
		 gmtime(&http_res_msg->file_stat.st_mtime));

	snprintf(http_res_msg->header, sizeof(http_res_msg->header),
		 "%s %d %s\r\n"
		 "Cache-Control: max-age=120\r\n"
		 "Connection: close\r\n"
		 "Content-type: %s\r\n"
		 "Content-length: %ld\r\n"
		 "Last-Modified: %s\r\n"
		 "Server: %s\r\n"
		 "\r\n",
		 HTTP_VER, http_res_msg->status->status_code,
		 http_res_msg->status->status_name, http_res_msg->mime->value,
		 http_res_msg->file_stat.st_size, http_res_msg->file_mtime,
		 SERVER_NAME);

	send(http_res_msg->client_sockfd, http_res_msg->header,
	     strlen(http_res_msg->header), MSG_NOSIGNAL);

	if (http_res_msg->status->status_code == HTTP_HEAD)
		goto reply_end;

	int file_fd = open(http_res_msg->filename, O_RDONLY, 0);
	char *file_addr = mmap(NULL, http_res_msg->file_stat.st_size, PROT_READ,
			       MAP_PRIVATE, file_fd, 0);
	close(file_fd);
	send(http_res_msg->client_sockfd, file_addr,
	     http_res_msg->file_stat.st_size, MSG_NOSIGNAL);
	munmap(file_addr, http_res_msg->file_stat.st_size);
reply_end:
	return;
}

static void http_reply_err(struct http_response_msg *http_res_msg) {

	snprintf(http_res_msg->body, sizeof(http_res_msg->body),
		 "<html>"
		 "<head><title>Not Found</title></head>"
		 "<body><h1>[404] Not Found</h1></body>"
		 "</html>");
	snprintf(http_res_msg->header, sizeof(http_res_msg->header),
		 "%s %d %s\r\n"
		 "Connection: close\r\n"
		 "Content-type: %s\r\n"
		 "Content-length: %ld\r\n"
		 "Server: %s\r\n"
		 "\r\n",
		 HTTP_VER, http_res_msg->status->status_code,
		 http_res_msg->status->status_name, http_res_msg->mime->value,
		 strlen(http_res_msg->body), SERVER_NAME);
	send(http_res_msg->client_sockfd, http_res_msg->header,
	     strlen(http_res_msg->header), MSG_NOSIGNAL);
	send(http_res_msg->client_sockfd, http_res_msg->body,
	     strlen(http_res_msg->body), MSG_NOSIGNAL);
}
