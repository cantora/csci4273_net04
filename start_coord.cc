#include <cstdio>
#include <cstring>

#include "coord.h"
#include "sock.h"

using namespace net04;
using namespace net01;

void start_coord(struct sockaddr_in *sin) {
	coord c(sin);

	printf("port: %d\n", c.port() );

	
	while(1) { 
		sleep(1);
	}
}

void usage(int argc, char *argv[]) {
	printf("usage: %s NODE1.NODE2.COST [NODE1.NODE2.COST, ...]\n", argv[0]);
}

int parse_edge(char *str, coord::edge &e) {
	char *tok = strtok(str, ".");
	int stuff[3];
	int i;
	
	for(i = 0; i < 3; i++) {
		if(tok == NULL) return -1;

		stuff[i] = strtol(tok, NULL, 10);
		if(stuff[i] < 1 || stuff[i] > 255) return -1;

		tok = strtok(NULL, ".");
	}	

	return 0;
}

int main(int argc, char *argv[]) {
	struct sockaddr_in sin;
	int i;
	coord::edge e;

	if(argc < 2) {
		usage(argc, argv);
		exit(1);
	}

	for(i = 1; i < argc; i++) {
		if(parse_edge(argv[i], e) != 0) {
			printf("could not parse %s\n", argv[i]);
			usage(argc, argv);
			exit(1);
		}
	}
	
	return 0;

	sock::passive_sin(54545, &sin);

	start_coord(&sin);

	return 0;
}
