#include "network_utils.h"

int
main(int argc, char **argv) {

	printf("Dest = [%s,%d] \n", argv[1], atoi(argv[2]));
	send_udp_msg(argv[1], atoi(argv[2]), argv[3], strlen( argv[3] ));
	return 0;
}
