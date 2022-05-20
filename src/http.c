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
#include "cserver.h"
#include "http.h"
#include "util.h"
#define MAXLEN 8192
#define MAX_FILE_NAME_LEN 64
#define HTTP_VER "HTTP/1.1" /* HTTP Start Line */

#if DEBUG == 1
#include <pthread.h>
#endif

struct http_req_msg {
	int client_sockfd;
	char recv_msg[MAXLEN];
	char *request_line, *header, *body;
	// Request line
	int method;
	char *http_ver;

	// Request header
	struct list_head *headers;
};

struct http_res_msg {
	int client_sockfd;
	struct http_status_table *status;
	char filename[MAX_FILE_NAME_LEN];
	struct stat file_stat;
	char file_mtime[36];
	struct mime_type *mime;
	char header[MAXLEN];
	char body[MAXLEN];
};

enum setting_option {
	IP = 0,
	PORT = 1,
	FDIR = 2,
	HOMEPAGE = 3
};

#define SETTINGS(FIELD) server_settings[FIELD].val

static struct setting {
	char *key;
	char val[1024];
} server_settings[] = {
	{"ip", "localhost"},
	{"port", "8081"},
	{"fdir", "www"},
	{"homepage", "index.html"}
};

enum http_status_idx{
	OK_IDX,
	NOT_MODIFIED_IDX,
	NOT_FOUND_IDX
};

static struct http_status_table http_status_table[] = {
	{HTTP_OK, "OK"},
	{HTTP_NOT_MODIFIED, "Not Modified"},
	{HTTP_NOT_FOUND, "Not Found"},
};

struct mime_type {
	const char *type, *value;
};

static struct mime_type mime_type[] = {{".html", "text/html"},
				       {".json", "application/json"},
				       {NULL, "text/plain"}};

static void http_parse(struct http_req_msg *, struct http_res_msg *);

static void http_parse_request_line(struct http_req_msg *,
				    struct http_res_msg *);

static void http_parse_request_header(struct http_req_msg *,
				      struct http_res_msg *);

static void http_parse_request_body(struct http_req_msg *,
				    struct http_res_msg *);

static void http_reply(struct http_req_msg *, struct http_res_msg *);

static void http_reply_err(struct http_res_msg *);

#if DEBUG == 1
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static int requests = 0;
#endif

static void parse_arg(char *arg) {
	for (int i = 0; i < sizeof(server_settings) / sizeof(struct setting); i++) {
		if (strstr(arg, server_settings[i].key) != NULL) {
			sscanf(arg, "%*[A-Za-z]=%s", server_settings[i].val);
			return;
		}
	}
}

int init_http_settings(int argc, char **argv) {
	for (int i = 1; i < argc; i++) {
		parse_arg(argv[i]);
	}
	int server_sockfd = init_server(SETTINGS(IP), atoi(SETTINGS(PORT)), LISTEN_NUMS);
	return server_sockfd;
}

void http_request(void *sockfd) {
#if DEBUG == 1
	pthread_mutex_lock(&lock);
	requests++;
	pthread_mutex_unlock(&lock);
#endif

	struct http_req_msg http_req_msg = {0};
	struct http_res_msg http_res_msg = {0};
	http_req_msg.client_sockfd = http_res_msg.client_sockfd = (int)SOCK_FD_MASK(sockfd);
	recv(http_req_msg.client_sockfd, http_req_msg.recv_msg,
	     sizeof(http_req_msg.recv_msg), 0);

	if (!strcmp(http_req_msg.recv_msg, "")) {
		printf("sockfd(%d) No any message\n",
		       http_req_msg.client_sockfd);
		goto close;
	}

	http_parse(&http_req_msg, &http_res_msg);
	http_reply(&http_req_msg, &http_res_msg);

close:
	shutdown((int)SOCK_FD_MASK(sockfd), SHUT_RDWR);
	close(http_req_msg.client_sockfd);
}

static void http_parse(struct http_req_msg *http_req_msg,
		       struct http_res_msg *http_res_msg) {
	char *parser = http_req_msg->recv_msg;

	http_req_msg->request_line = parser;

	http_req_msg->header = strstr(http_req_msg->recv_msg, "\r\n") + sizeof("\r\n");
	http_req_msg->body = strstr(http_req_msg->recv_msg, "\r\n\r\n") + sizeof("\r\n\r\n");

	http_parse_request_line(http_req_msg, http_res_msg);
	http_parse_request_header(http_req_msg, http_res_msg);
	http_parse_request_body(http_req_msg, http_res_msg);
}

static void http_parse_request_line(struct http_req_msg *http_req_msg,
				    struct http_res_msg *http_res_msg) {
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

	strcat(http_res_msg->filename, server_settings[FDIR].val);
	strncat(http_res_msg->filename, uri, (size_t)(http_ver - uri) - 1);

	if (!strncmp(uri, "/", (size_t)(http_ver - uri) - 1))
		strcat(http_res_msg->filename, server_settings[HOMEPAGE].val);

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

static void http_parse_request_header(struct http_req_msg *http_req_msg,
				      struct http_res_msg *http_res_msg) {
	char header_field[32], header_val[1024];
	
}

static void http_parse_request_body(struct http_req_msg *http_req_msg,
				    struct http_res_msg *http_res_msg) {
	if (http_req_msg->method == HTTP_POST) {
		printf("%s\n", http_req_msg->body);
	}
}

static void http_reply(struct http_req_msg *http_req_msg, struct http_res_msg *http_res_msg) {

	if (stat(http_res_msg->filename, &http_res_msg->file_stat) < 0) {
		fprintf(stderr, "File: (%s) Not Found\n", http_res_msg->filename);
		http_res_msg->status = http_status_table + NOT_FOUND_IDX;
		http_res_msg->mime = mime_type;
		http_reply_err(http_res_msg);
		goto reply_end;
	}

	http_res_msg->status = http_status_table + OK_IDX;

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

	if (http_req_msg->method == HTTP_HEAD)
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

static void http_reply_err(struct http_res_msg *http_res_msg) {

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
