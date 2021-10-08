#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include "util.h"


void handle_error(int err) {
    if (err < 0) {
        fprintf(stderr, "%s\n", strerror(errno));
        exit(0);
    }
}

int init_server_sockfd(int listen_nums) {
    int server_sockfd = 0;
    struct sockaddr_in server_info;
    handle_error(server_sockfd = socket(AF_INET, SOCK_STREAM, 0));
    int reuse = 1;
    handle_error(setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR,
                            (const char *)&reuse, sizeof(int)));
    memset(&server_info, 0, sizeof(server_info));
    server_info.sin_family = PF_INET;
    server_info.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    server_info.sin_port = htons(8000);
    handle_error(bind(server_sockfd, (struct sockaddr *)(&server_info),
                      sizeof(server_info)));
    handle_error(listen(server_sockfd, listen_nums));

    return server_sockfd;
}