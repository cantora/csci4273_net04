#include "node.h"

#include "net04_common.h"
#include "sock.h"
#include "proto_coord.h"
#include "proto_base.h"

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

void node::send_coord_msg(const char *buf, int buflen) const {
	proto_coord::header_t *reply = (proto_coord::header_t *) buf;

	assert(buflen >= sizeof(proto_coord::header_t) );

	proto_coord::hton_hdr(reply);
	proto_base::send_udp_msg(m_coord_socket, m_coord_addr, buflen, buf);
}

void node::request_coord_init() const {
	proto_coord::header_t hdr;

	proto_coord::request_coord_init(&hdr, m_node_id);

	send_coord_msg((char *)&hdr, sizeof(hdr));
}

void node::listen_coord(void *instance) {
	node *n = (node *) instance;
	int msglen;
	char msgbuf[proto_coord::max_msg_len];

	NET04_LOG("listen_coord: start\n");
	set_nonblocking(n->m_coord_socket);

	n->request_coord_init();

	while(1) {
		msglen = recvfrom(n->m_coord_socket, msgbuf, sizeof(msgbuf), 0, NULL, NULL);
		if( msglen < 0) {
			if(errno == EAGAIN) {
				usleep(100000);
				continue;	
			}

			FATAL(NULL);
		}
		else if(msglen == 0) {
			FATAL("socket was closed");
		}

		n->on_coord_msg(msglen, msgbuf);
	}

}

void node::on_coord_msg(int msglen, const char *msg) {
	NET04_LOG("received %d byte message from coord: ", msglen);

	print_hex_bytes(msg, msglen);

	printf("\n");
}

void node::listen_dv(void *instance) {
	node *n = (node *) instance;

	NET04_LOG("listen_dv: start\n");
	

	NET04_LOG("listen_dv: stop\n");
} 