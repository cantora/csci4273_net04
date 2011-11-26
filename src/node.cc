#include "node.h"

#include "net04_common.h"
#include "sock.h"

using namespace net04;
using namespace net02;
using namespace net01;
using namespace std;

node::node(node_id_t node_id, const struct sockaddr_in *coord_addr, 
	struct sockaddr_in *coord_sin, struct sockaddr_in *dv_sin)
	: m_coord_addr(coord_addr), m_dv_sin(dv_sin), m_coord_sin(coord_sin), m_node_id(node_id), 
	  m_pool(new thread_pool(2 + 8) ) {

	m_dv_socket = sock::bound_udp_socket(m_dv_sin);
	m_coord_socket = sock::bound_udp_socket(m_coord_sin);

	while(m_pool->dispatch_thread(listen_coord, this, NULL) != 0) {
		usleep(1000);
	}

	while(m_pool->dispatch_thread(listen_dv, this, NULL) != 0) {
		usleep(1000);
	}
}

node::~node() {
	delete m_pool;
	
	close(m_dv_socket);
	close(m_coord_socket);
}

void node::listen_coord(void *instance) {
	node *n = (node *) instance;

	NET04_LOG("listen_coord: start\n");

	
	NET04_LOG("listen_coord: stop\n");
}

void node::listen_dv(void *instance) {
	node *n = (node *) instance;

	NET04_LOG("listen_dv: start\n");

	

	NET04_LOG("listen_dv: stop\n");
} 