#include <cstdio>

#include "node.h"
#include "sock.h"

using namespace net04;
using namespace net01;

void start_node(node::node_id_t node_id, struct sockaddr_in *coord_sin, struct sockaddr_in *dv_sin, struct sockaddr_in *coord_addr) {
	node n(node_id, coord_addr, coord_sin, dv_sin);

	usleep(500000);
}

void usage(int argc, char *argv[]) {
	printf("usage: %s ID PORT [HOSTNAME] \n", argv[0]);
}

int main(int argc, char *argv[]) {
	struct sockaddr_in coord_sin;
	struct sockaddr_in dv_sin;
	struct sockaddr_in coord_addr;
	unsigned short coord_port,id;
	const char *coord_host = "localhost";
	
	
	if(argc < 3 || argc > 4) {
		usage(argc, argv);
		exit(1);
	}
	
	id = strtol(argv[1], NULL, 10);
	if(id < 1 || id > 255) {
		printf("invalid node id %d. must be in the range of 1-255", id);
		exit(1);
	}

	coord_port = strtol(argv[2], NULL, 10);
	
	if(argc == 4) {
		coord_host = argv[3];
	}
		
	sock::passive_sin(0, &coord_sin);
	sock::passive_sin(0, &dv_sin);
	sock::host_sin(coord_host, coord_port, &coord_addr);

	printf("start node\n");

	start_node(id, &coord_sin, &dv_sin, &coord_addr);

	return 0;
}
