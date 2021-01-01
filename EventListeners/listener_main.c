
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

void *
main_menu(void *arg) {

	printf("1. Start a new UDP listening Server\n");
	printf("2. Cancel a UDP Server\n");
	printf("3. Stop process\n");
	
	int ch;

	do {

		scanf("%d", &ch);
		switch(ch) {

			case 1:
			case 2:
				break;
			case 3:
				pthread_cancel(*listener1);
				free(listener1);
				listener1 = NULL;
				pthread_cancel(*listener2);
				free(listener2);
				listener2 = NULL;
				pthread_exit(0);
			default:;
		}
	}while(1);
}

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

	pthread_t user_interaction_thread;
	pthread_create(&user_interaction_thread, 0, main_menu, 0);

	pthread_exit(0);	
	return 0;
}
