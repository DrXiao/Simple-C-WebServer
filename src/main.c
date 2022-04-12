#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "cserver.h"
#include "http.h"
#include "util.h"
#include "threadpool.h"
/*
 * socket(int __domain, int __type, int __protocol)
 * __domain     : AF_INET       - IPv4
 * __type       : SOCK_STREAM   - TCP
 * __protocol   : 0             - Let kernel select corresponding protocol for
 *                                __type.
 *
 * */

int main(int argc, char *argv[]) {
	// Server
	int server_sockfd = init_http_settings(argc, argv);
	 
	threadpool_t *pool;
	assert((pool = threadpool_create(THREAD, QUEUE, 0)) != NULL);
	printf("Pool started with %d threads and queue size of %d\n", THREAD,
	       QUEUE);
	
	// Client
	long long int client_sockfd = 0;
	struct sockaddr_in client_info;
	socklen_t addrlen = sizeof(client_info);

	while (true) {
		handle_error((client_sockfd =
				      accept(server_sockfd,
					     (struct sockaddr *)(&client_info),
					     &addrlen)));
		threadpool_add(pool, http_request, (void *)SOCK_FD_MASK(client_sockfd), 0);
	}

	assert(threadpool_destroy(pool, 0) == 0);
	printf("Server is terminated.\n");
	return 0;
}
