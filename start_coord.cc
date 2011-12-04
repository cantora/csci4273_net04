#include <cstdio>
#include <cstring>

#include <list>
#include <memory>
#include "coord.h"
#include "sock.h"
#include "net04_common.h"

using namespace net04;
using namespace net01;

void usage(int argc, char *argv[]) {
	printf("usage: %s NODE1.NODE2.COST [NODE1.NODE2.COST, ...]\n", argv[0]);
}

int parse_edge(char *str, coord::edge &e, cost_t &cost) {
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

	e.first = stuff[0];
	e.second = stuff[1];
	cost = stuff[2];

	if(e.first == e.second) {
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
	printf("\tr\t\t\t\treset nodes and exit\n");
	printf("\th\t\t\t\tprint this message\n");
}

bool valid_node_id(int id) {

	if(id < 0 || id > 255) {
		return false;
	}

	return true;
}

void cost_change(coord *c, node_id_t n1, node_id_t n2, cost_t cost) {
	if(c->link_cost_change(n1, n2, cost) != 0) {
		printf("no such link in network\n");
	}
	else {
		printf("notified network of link change\n");
	}
}

int parse_n_uint8s(char *str, uint8_t *arr, int n, char **end) {
	char *tok = strtok(str, " ");
	std::auto_ptr<int> p1(new int[n]);
	int i;
	
	for(i = 0; i < n; i++) {
		tok = strtok(NULL, " ");

		if(tok == NULL || (tok[0] > 0x39) || (tok[0] < 0x30) ) {
			printf("invalid argument: %s\n", tok);
			return -1;
		}

		(&*p1)[i] = strtol(tok, NULL, 10);

		if( ((&*p1)[i] < 0) || ((&*p1)[i] > 255) ) {
			printf("invalid value: %d", (&*p1)[i]);
			return -1;
		}

		arr[i] = (&*p1)[i];
	}

	if(end != NULL) {
		*end = tok + strlen(tok) + 1;
	}

	return 0;
}

void do_cost_change(coord *c, char *str) {
	uint8_t arr[3];
	int status;

	status = parse_n_uint8s(str, arr, 3, NULL);

	if( (status != 0) || !valid_node_id(arr[0]) || !valid_node_id(arr[1]) || arr[2] == 0 ) {
		printf("invalid arguments\n");
	}

	cost_change(c, arr[0], arr[1], arr[2]);
}

void do_link_fail(coord *c, char *str) {
	uint8_t arr[2];
	int status;

	status = parse_n_uint8s(str, arr, 2, NULL);

	if( (status != 0) || !valid_node_id(arr[0]) || !valid_node_id(arr[1]) ) {
		printf("invalid arguments\n");
		return;
	}

	cost_change(c, arr[0], arr[1], 0);
}

void do_print_table(coord *c, char *str) {
	char *tok = strtok(str, " ");
	int id;

	tok = strtok(NULL, " ");
	
	if(tok == NULL || (tok[0] > 0x39) || (tok[0] < 0x30) ) {
		printf("invalid node_id\n");
		return;
	}

	id = strtol(tok, NULL, 10);

	if(valid_node_id(id) == false) {
		printf("invalid node_id %d\n", id);
		return;
	}

	printf("node %d:\n", id);
	c->print_table(id);	

}

void do_print_all_tables(coord *c, const char *str) {
	c->print_all_tables();
}

void do_send_message(coord *c, char *str) {
	node_id_t arr[2];
	int status;
	char *tok, *end;

	status = parse_n_uint8s(str, arr, 2, &end);

	if( (status != 0) || !valid_node_id(arr[0]) || !valid_node_id(arr[1]) ) {
		printf("invalid arguments\n");
		return;
	}

		
	printf("send message from node %d to node %d: %s\n", arr[0], arr[1], end);
	
	if(c->send(5, arr[0], arr[1], end, strlen(end)-1) != 0) { // -1 for the newline
		if(errno == ETIMEDOUT) {
			printf("error: response timeout\n");
			return;
		}
		else {
			FATAL("c->send");
		}
	}
}

void do_net_reset(coord *c, const char *str) {
	c->bcast_reset();
	return;
}

void do_cmd(coord *c, char *str) {
	char *p = str;
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
		case 'r' :
			do_net_reset(c, p);
			exit(0);
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
	coord::edge e(1,2); // dummy values
	cost_t cost;
	coord c(sin);
	char buf[512];
	
	for(i = 1; i < argc; i++) {
		len = strlen(argv[i]);
		strncpy(buf, argv[i], ((len > 512)? 512 : len) );
		buf[len] = 0x00;

		if(parse_edge(buf, e, cost) != 0) {
			printf("could not parse %s\n", argv[i]);
			usage(argc, argv);
			exit(1);
		}
		
		printf("add edge: %d<-(%d)->%d\n", e.first, cost, e.second);
		c.add_edge(e, cost);
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
