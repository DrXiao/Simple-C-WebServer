#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "http.h"
#include "util.h"
#define WAY 3
#define LISTEN_NUMS 1024

#if WAY == 2
#include <pthread.h>
#endif

#if WAY == 3
#include "threadpool.h"
#define THREAD 32
#define QUEUE 1024
#endif
/*
 * socket(int __domain, int __type, int __protocol)
 * __domain     : AF_INET       - IPv4
 * __type       : SOCK_STREAM   - TCP
 * __protocol   : 0             - Let kernel select corresponding protocol for
 * __type.
 *
 * */



int main(int argc, char *argv[]) {
    // Server
    int server_sockfd = init_server_sockfd(LISTEN_NUMS);

#if WAY == 1
    int pid = 0;
#elif WAY == 3
    threadpool_t *pool;
    assert((pool = threadpool_create(THREAD, QUEUE, 0)) != NULL);
    printf("Pool started with %d threads and queue size of %d\n", THREAD,
           QUEUE);
#endif
    int client_sockfd = 0;
    struct sockaddr_in client_info;
    int addrlen = sizeof(client_info);

    printf("Server run ...\n"
           "press Ctrl + C to exit...\n");

    while (true) {
        handle_error((client_sockfd =
                          accept(server_sockfd,
                                 (struct sockaddr *)(&client_info), &addrlen)));
#if WAY == 1
        handle_error((pid = fork()));
        if (pid == 0) {
            http_request(&client_sockfd);
            exit(0);
        }
#elif WAY == 2
        pthread_t thread;
        handle_error(pthread_create(
            &thread, NULL, (void *(*)(void *))http_request, (void *)client_sockfd));
#elif WAY == 3
        threadpool_add(pool, http_request, (void *)client_sockfd, 0);
#endif
    }

#if WAY == 3
    assert(threadpool_destroy(pool, 0) == 0);
#endif
    printf("Server is terminated.\n");
    return 0;
}