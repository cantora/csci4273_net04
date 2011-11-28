#include <cstdio>
#include <cstring>

#include <list>

#include "coord.h"
#include "sock.h"
#include "net04_common.h"

using namespace net04;
using namespace net01;

void usage(int argc, char *argv[]) {
	printf("usage: %s NODE1.NODE2.COST [NODE1.NODE2.COST, ...]\n", argv[0]);
}

int parse_edge(char *str, coord::edge &e) {
	char *tok;
	int stuff[3];
	int i;

	//printf("parse %s\n", str);	

	tok = strtok(str, ".");
	for(i = 0; i < 3; i++) {
		if(tok == NULL) {
			printf("incorrect syntax for link specification\n");
			return -1;
		}

		stuff[i] = strtol(tok, NULL, 10);
		if(stuff[i] < 1 || stuff[i] > 255) {
			printf("token %s specifies the out of range node id or cost %d\n", tok, stuff[i]);
			return -1;
		}

		tok = strtok(NULL, ".");
	}	

	e.n1 = stuff[0];
	e.n2 = stuff[1];
	e.cost = stuff[2];

	if(e.n1 == e.n2) {
		printf("link specifies the same node for each end\n");
		return -1;
	}

	return 0;
}

void command_usage() {
	printf("command usage:\n");
	printf("\tc NODE1 NODE2 COST\t\tcost change\n");
	printf("\tf NODE1 NODE2\t\t\tlink failure\n");
	printf("\tp NODE\t\t\t\tprint forwarding table of NODE\n");
	printf("\ta\t\t\t\tprint forwarding table of all nodes\n");
	printf("\ts NODE1 NODE2 DATA\t\tsend DATA message from NODE1 to NODE2\n");
	printf("\th\t\t\t\tprint this message\n");
}

void do_cost_change(coord *c, const char *str) {
	printf("do_cost_change\n");
}

void do_link_fail(coord *c, const char *str) {
	printf("do_link_fail\n");
}

void do_print_table(coord *c, const char *str) {
	printf("do_print_table\n");
}

void do_print_all_tables(coord *c, const char *str) {
	printf("do_print_all_tables\n");
}

void do_send_message(coord *c, const char *str) {
	printf("do_send_message\n");
}

void do_cmd(coord *c, const char *str) {
	const char *p = str;
	while(*p != 0x00 && *p == ' ') p++;
 
	if(*p == 0x00 || *p == '\n') return;

	switch(*p) {
		case 'c' : 
			do_cost_change(c, p);
			break;
		case 'f' : 
			do_link_fail(c, p);
			break;
		case 'p' : 
			do_print_table(c, p);
			break;
		case 'a' : 
			do_print_all_tables(c, p);
			break;
		case 's' : 
			do_send_message(c, p);
			break;
		case 'h' : 
			command_usage();
			break;
		default :
			printf("unknown command: %s", p);
			return;
	}

	return;
}

void start_coord(struct sockaddr_in *sin, int argc, char *argv[]) {
	int i,len;
	coord::edge e;
	coord c(sin);
	char buf[512];
	
	for(i = 1; i < argc; i++) {
		len = strlen(argv[i]);
		strncpy(buf, argv[i], ((len > 512)? 512 : len) );
		buf[len] = 0x00;

		if(parse_edge(buf, e) != 0) {
			printf("could not parse %s\n", argv[i]);
			usage(argc, argv);
			exit(1);
		}
		
		printf("add edge: %d<-(%d)->%d\n", e.n1, e.cost, e.n2);
		c.add_edge(e);
	}
	
	printf("port: %d\n", c.port() );
	
	while(1) { 
		printf(" waiting for nodes: %d edges, %d nodes", c.edges(), c.nodes() );
		for(i = 0; i < 10; i++) printf(" ");
		printf("\r");
		fflush(stdout);
		
		if(c.network_ready() ) {
			break;
		}
		usleep(500000);
	}

	printf("all nodes registered. network is ready...\n");
	command_usage();
	
	while(1) {
		printf("> "); fflush(stdout);
		if((fgets(buf, sizeof(buf), stdin) == NULL) && ferror(stdin)) {
			FATAL("error reading input\n");
		}

		//printf("command: %s\n", buf);
		do_cmd(&c, buf);
	}
}



int main(int argc, char *argv[]) {
	struct sockaddr_in sin;
	
	if(argc < 2) {
		usage(argc, argv);
		exit(1);
	}

	
	sock::passive_sin(54545, &sin);

	start_coord(&sin, argc, argv);

	return 0;
}
