#include <cstdio>

#include "coord.h"
#include "sock.h"

using namespace net04;
using namespace net01;

/* asdf */

void start_coord(struct sockaddr_in *sin) {
	coord c(sin);

	printf("port: %d\n", c.port() );

	pause();
}

int main(int argc, char *argv[]) {
	struct sockaddr_in sin;
	
	sock::passive_sin(0, &sin);

	start_coord(&sin);

	return 0;
}
