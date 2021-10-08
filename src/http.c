#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "http.h"
#define MAXLEN 4096

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

struct mime_type_t {
    const char *type;
    const char *value;
};

static struct mime_type_t mime[] = {{".html", "text/html"},
                                    {NULL, "text/plain"}};

void http_request(void *sockfd) {
    int client_sockfd = (int)sockfd;
    char recv_msg[2048] = "";
    recv(client_sockfd, recv_msg, sizeof(recv_msg), 0);
    http_parse(recv_msg);
    http_reply(client_sockfd);
    shutdown(client_sockfd, SHUT_RDWR);
    close(client_sockfd);
}

void http_parse(const char *http_req) {
    char *start_line, *header, *body;
    // printf("%s\n", http_req);
}

void http_reply(int client_sockfd) {
    char header[MAXLEN], body[MAXLEN];
    sprintf(body, "<html>"
                  "<head><title>Hello world</title></head>"
                  "<body><h1>Hello world</h1></body>"
                  "</html>");
    sprintf(header,
            "%s 200 OK\r\n"
            "Server: %s\r\n"
            "Content-type: text/html\r\n"
            "Connection: close\r\n"
            "Content-length: %ld\r\n\r\n",
            HTTP_VER, "CServer", strlen(body));
    send(client_sockfd, header, strlen(header), MSG_NOSIGNAL);
    send(client_sockfd, body, strlen(body), MSG_NOSIGNAL);
}