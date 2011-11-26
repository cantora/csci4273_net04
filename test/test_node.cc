#include <cstdio>

#include "node.h"
#include "sock.h"

using namespace net04;
using namespace net01;

int main() {
	node *n;
	struct sockaddr_in coord_sin;
	struct sockaddr_in dv_sin;
	struct sockaddr_in coord_addr;

	short coord_port = 1234;
	const char* coord_host = "localhost";
		
	printf("test_node...\n");

	sock::passive_sin(0, &coord_sin);
	sock::passive_sin(0, &dv_sin);
	sock::host_sin(coord_host, coord_port, &coord_addr);

	n = new node(0, &coord_addr, &coord_sin, &dv_sin);

	delete n;

	return 0;
}