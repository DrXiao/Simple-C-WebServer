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
#define MAXLEN 2048
#define MAX_FILE_NAME_LEN 64
#if DEBUG == 1
#include <pthread.h>
#endif

/* HTTP Start Line */
#define HTTP_VER "HTTP/1.1"

/* HTTP General Headers
 *
 * Cache-Control
 * Connection
 * Date
 * Pragma
 * Trailer
 * Transfer-Encoding
 * Upgrade
 * Via
 * Warning
 * */

/* HTTP Entity Headers
 *
 * Allow
 * Content-Encoding
 * Content-Language
 * Content-Length
 * Content-Location
 * Content-MD5
 * Content-Range
 * Content-Type
 * Expires
 * Last_Modified
 * extension-header=message-header
 * */

/* HTTP Response headers
 *
 * Accept-Ranges
 * Age
 * ETag
 * Location
 * Proxy-Authenticate
 *
 * Retry-After
 * Server
 * Vary
 * WWW-Authenticate
 * */

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

struct http_status_table {
    uint16_t status;
    const char *status_name;
};

struct http_request_msg_t {
    int client_sockfd;
    char recv_msg[MAXLEN];
    char *request_line, *header, *body;
    // Request line
    int method;
    char filename[MAX_FILE_NAME_LEN];
    const char *http_ver;
    // Request header
};

struct http_response_msg_t {
    int client_sockfd;
    int status;
    char *filename;
    struct stat file_stat;
    char header[MAXLEN];
    char body[MAXLEN];
};

struct mime_type_t {
    const char *type;
    const char *value;
};

static char *file_dir = "www";

struct http_status_table http_status_table[] = {
    {HTTP_OK, "OK"},
    {HTTP_NOT_MODIFIED, "Not Modified"},
    {HTTP_NOT_FOUND, "Not Found"},
};

static struct mime_type_t mime[] = {{".html", "text/html"},
                                    {NULL, "text/plain"}};

static char *request_methods[] = {"GET", "POST", NULL};

static void http_parse(struct http_request_msg_t *,
                       struct http_response_msg_t *);

static void http_parse_request_line(struct http_request_msg_t *);

static void http_parse_request_header(struct http_request_msg_t *);

static void http_parse_request_body(struct http_request_msg_t *);

static void http_reply(struct http_response_msg_t *);

static void http_reply_err(struct http_response_msg_t *);

#if DEBUG == 1
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static int requests = 0;
#endif

void http_request(void *sockfd) {
#if DEBUG == 1
    pthread_mutex_lock(&lock);
    requests++;
    pthread_mutex_unlock(&lock);
#endif

    struct http_request_msg_t http_req_msg;
    struct http_response_msg_t http_res_msg;
    memset(&http_req_msg, 0, sizeof(http_req_msg));
    memset(&http_res_msg, 0, sizeof(http_res_msg));
    http_req_msg.client_sockfd = http_res_msg.client_sockfd = (int)sockfd;
    recv(http_req_msg.client_sockfd, http_req_msg.recv_msg,
         sizeof(http_req_msg.recv_msg), 0);

    if (!strcmp(http_req_msg.recv_msg, "")) {
        printf("sockfd(%d) No any msg\n", http_req_msg.client_sockfd);
        goto close;
    }

    http_parse(&http_req_msg, &http_res_msg);
    http_reply(&http_res_msg);

close:
    shutdown(http_req_msg.client_sockfd, SHUT_RDWR);
    close(http_req_msg.client_sockfd);
}

static void http_parse(struct http_request_msg_t *http_req_msg,
                       struct http_response_msg_t *http_res_msg) {
    char *parser = http_req_msg->recv_msg;

    http_req_msg->request_line = parser;

    parser = strstr(http_req_msg->recv_msg, "\r\n");
    http_req_msg->header = parser + 2;

    parser = strstr(http_req_msg->recv_msg, "\r\n\r\n");
    http_req_msg->body = parser + 4;

    // TODO: Parse start line, header and body
    http_parse_request_line(http_req_msg);
    http_res_msg->filename = http_req_msg->filename;
}

static void http_parse_request_line(struct http_request_msg_t *http_req_msg) {
    char *method, *uri, *http_ver;

    http_req_msg->request_line = strtok(http_req_msg->request_line, "\r\n");

    method = strtok(http_req_msg->request_line, " ");

    uri = strtok(NULL, " ");

    http_ver = strtok(NULL, " ");

    snprintf(http_req_msg->filename, MAX_FILE_NAME_LEN, "%s%s", file_dir, uri);

    if (!strcmp(uri, "/"))
        strcat(http_req_msg->filename, "homepage.html");
}

static void http_parse_request_header(struct http_request_msg_t *http_req_msg) {
    return;
}

static void http_parse_request_body(struct http_request_msg_t *http_req_msg) {
    return;
}

static void http_reply(struct http_response_msg_t *http_res_msg) {

    if (stat(http_res_msg->filename, &http_res_msg->file_stat) < 0) {
        printf("File: (%s) Not Found\n", http_res_msg->filename);
        http_reply_err(http_res_msg);
        return;
    }

    sprintf(http_res_msg->header,
            "%s 200 OK\r\n"
            "Server: %s\r\n"
            "Content-type: text/html\r\n"
            "Connection: close\r\n"
            "Content-length: %ld\r\n\r\n",
            HTTP_VER, "CServer", http_res_msg->file_stat.st_size);

    int file_fd = open(http_res_msg->filename, O_RDONLY, 0);
    char *file_addr = mmap(NULL, http_res_msg->file_stat.st_size, PROT_READ,
                           MAP_PRIVATE, file_fd, 0);
    close(file_fd);
    send(http_res_msg->client_sockfd, http_res_msg->header,
         strlen(http_res_msg->header), MSG_NOSIGNAL);
    send(http_res_msg->client_sockfd, file_addr,
         http_res_msg->file_stat.st_size, MSG_NOSIGNAL);
    munmap(file_addr, http_res_msg->file_stat.st_size);
}

static void http_reply_err(struct http_response_msg_t *http_res_msg) {

    sprintf(http_res_msg->body, "<html>"
                                "<head><title>Not Found</title></head>"
                                "<body><h1>[404] Not Found</h1></body>"
                                "</html>");
    sprintf(http_res_msg->header,
            "%s 404 Not Found\r\n"
            "Server: %s\r\n"
            "Content-type: text/html\r\n"
            "Connection: close\r\n"
            "Content-length: %ld\r\n\r\n",
            HTTP_VER, "CServer", strlen(http_res_msg->body));
    send(http_res_msg->client_sockfd, http_res_msg->header,
         strlen(http_res_msg->header), MSG_NOSIGNAL);
    send(http_res_msg->client_sockfd, http_res_msg->body,
         strlen(http_res_msg->body), MSG_NOSIGNAL);
}