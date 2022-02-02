#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "http.h"
#include "util.h"

int init_server(const char *ip, uint16_t port, int listen_nums) {
	int server_sockfd = 0;
	in_addr_t server_ip = 0;

	if (!strcmp(ip, "") || !strcmp(ip, "localhost") ||
	    !strcmp(ip, "127.0.0.1"))
		server_ip = INADDR_LOOPBACK;

	if (port == 0)
		port = 8081;

	struct sockaddr_in server_info;
	memset(&server_info, 0, sizeof(server_info));
	server_info.sin_family = PF_INET;
	server_info.sin_addr.s_addr = htonl(server_ip);
	server_info.sin_port = htons(port);

	int reuse = 1;
	handle_error(server_sockfd = socket(AF_INET, SOCK_STREAM, 0));
	handle_error(setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR,
				(const char *)&reuse, sizeof(int)));
	handle_error(bind(server_sockfd, (struct sockaddr *)(&server_info),
			  sizeof(server_info)));
	handle_error(listen(server_sockfd, listen_nums));

	printf("* Server runs on http://%s:%d\n"
	       "* Press Ctrl + C to exit\n",
	       ip, port);
	return server_sockfd;
}
