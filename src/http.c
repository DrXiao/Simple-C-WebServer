#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "http.h"
#include "util.h"
#define MAXLEN 4096
#define MAX_FILE_NAME_LEN 32

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

char *file_dir = "www/";

struct mime_type_t {
    const char *type;
    const char *value;
};

static struct mime_type_t mime[] = {{".html", "text/html"},
                                    {NULL, "text/plain"}};

void http_request(void *sockfd) {
    int client_sockfd = (int)sockfd;
    char recv_msg[MAXLEN] = "", filename[MAX_FILE_NAME_LEN] = "";
    recv(client_sockfd, recv_msg, sizeof(recv_msg), 0);
    http_parse(recv_msg, filename);
    http_reply(client_sockfd, filename);
    shutdown(client_sockfd, SHUT_RDWR);
    close(client_sockfd);
}

static void http_parse(char *http_req, char *ret_filename) {
    char *start_line, *header, *body;
    char *parser = http_req;

    start_line = parser = http_req;

    parser = strstr(parser, "\r\n") + 2;
    *(parser - 1) = *(parser - 2) = '\0';
    header = parser;
    
    parser = strstr(parser, "\r\n\r\n") + 4;
    *(parser - 1) = *(parser - 2) = *(parser - 3) = *(parser - 4) = '\0';
    body = parser;

    // TODO: Parse start line, header and body

#if DEBUG == 1
    printf("%s\n\n%s\n\n%s\n", start_line, header, body);
#endif

    snprintf(ret_filename, MAX_FILE_NAME_LEN, "%s%s", file_dir, "homepage.html");
}

static void http_reply(int client_sockfd, const char *filename) {
    char header[MAXLEN];
    
    struct stat file_stat;
    
    if (stat(filename, &file_stat) < 0) {
        printf("File Not Found\n");
        http_reply_err(client_sockfd);
        return;
    }

    sprintf(header,
            "%s 200 OK\r\n"
            "Server: %s\r\n"
            "Content-type: text/html\r\n"
            "Connection: close\r\n"
            "Content-length: %ld\r\n\r\n",
            HTTP_VER, "CServer", file_stat.st_size);

    int file_fd = open(filename, O_RDONLY, 0);
    char *file_addr = mmap(NULL, file_stat.st_size, PROT_READ, MAP_PRIVATE, file_fd, 0);
    close(file_fd);

    send(client_sockfd, header, strlen(header), MSG_NOSIGNAL);
    send(client_sockfd, file_addr, file_stat.st_size, MSG_NOSIGNAL);

    munmap(file_addr, file_stat.st_size);
}

static void http_reply_err(int client_sockfd) {
    char header[MAXLEN], body[MAXLEN];

    sprintf(body, "<html>"
                  "<head><title>Not Found</title></head>"
                  "<body><h1>[404] Not Found</h1></body>"
                  "</html>");
    sprintf(header,
            "%s 404 Not Found\r\n"
            "Server: %s\r\n"
            "Content-type: text/html\r\n"
            "Connection: close\r\n"
            "Content-length: %ld\r\n\r\n",
            HTTP_VER, "CServer", strlen(body));
    send(client_sockfd, header, strlen(header), MSG_NOSIGNAL);
    send(client_sockfd, body, strlen(body), MSG_NOSIGNAL);
}