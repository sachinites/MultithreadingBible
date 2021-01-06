
/*
 * visit : www.csepracticals.com for more courses and projects
 */

#include <stdio.h>
#include <stdint.h>
#include "network_utils.h"

void
pkt_recv_fn(
    char *pkt,
    uint32_t pkt_size,
    char *sender_ip,
    uint32_t port_no) {

    printf("%s() : pkt recvd = %s, pkt size = %u\n",
        __FUNCTION__, pkt, pkt_size);
}

pthread_t *listener1 = NULL;
pthread_t *listener2 = NULL;

int
main(int argc, char **argv) {

	printf("Listening on UDP port no 3000\n");
	listener1 = udp_server_create_and_start(
					"127.0.0.1",
					3000,
					pkt_recv_fn);
	
	printf("Listening on UDP port no 3001\n");
	listener2 = udp_server_create_and_start(
					"127.0.0.1",
					3001,
					pkt_recv_fn);

	pthread_exit(0);
	return 0;
}
