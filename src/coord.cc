#include "coord.h"

#include <cassert>

#include "net04_common.h"
#include "sock.h"
#include "proto_coord.h"
#include "proto_base.h"


using namespace net04;
using namespace net02;
using namespace net01;
using namespace std;

coord::coord(struct sockaddr_in *sin) : m_sin(sin), m_pool(new thread_pool(2 + 8) ) {
	m_socket = sock::bound_udp_socket(m_sin);

	while(m_pool->dispatch_thread(listen_node, this, NULL) != 0) {
		usleep(1000);
	}

}

coord::~coord() {
	delete m_pool;

	close(m_socket);	
}

void coord::listen_node(void *instance) {
	coord *c = (coord *) instance;
	char msgbuf[1024];
	int msglen;
	struct sockaddr_in sin;
	socklen_t sinlen = sizeof(struct sockaddr_in);

	memset(&sin, 0, sizeof(sockaddr_in) );

	NET04_LOG("listen_node: start\n");

	set_nonblocking(c->m_socket);

	while(1) {
		msglen = recvfrom(c->m_socket, msgbuf, sizeof(msgbuf), 0, (sockaddr *)&sin, &sinlen);
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

		assert(sinlen > 0);

		c->on_node_msg(msglen, msgbuf, &sin);
	}

	assert(false); // shouldnt get here
}

void coord::on_node_msg(int msglen, char *msg, const struct sockaddr_in *sin) {
	uint16_t type;
	node::node_id_t node_id;
	proto_coord::header_t *hdr = (proto_coord::header_t *) msg;	

	assert(sin != NULL);

	proto_coord::ntoh_hdr(hdr);

	type = proto_coord::msg_type(msg);
	node_id = proto_coord::msg_node_id(msg);

	NET04_LOG("received %d byte '%s' message from %s:%d (%d): ", msglen, proto_coord::type_str[type], inet_ntoa(sin->sin_addr), ntohs(sin->sin_port), node_id, sin->sin_addr );

	if(m_nodes.count(node_id) > 0) {
		NET04_LOG("%d already registered, reply with error.\n");
		reply_err(node_id);
		
		return;
	}
	else {
		m_nodes[node_id] = *sin;

		NET04_LOG("reply ok (network size: %d)\n", m_nodes.size());
		reply_ok(node_id);
	}
}

void coord::reply_net_size(node::node_id_t node_id) {
	char buf[sizeof(proto_coord::header_t) + sizeof(proto_coord::net_size_t)];
	proto_coord::header_t *reply = (proto_coord::header_t *) buf;
	
	proto_coord::reply_net_size(reply);
	
	assert(sizeof(proto_coord::net_size_t) == 1);
	*(buf + sizeof(proto_coord::header_t) ) = (proto_coord::net_size_t) m_nodes.size();
	
	reply_msg(node_id, buf, sizeof(buf) );
}

void coord::reply_err(node::node_id_t node_id) {
	char buf[sizeof(proto_coord::header_t)];
	proto_coord::header_t *reply = (proto_coord::header_t *) buf;
	
	proto_coord::reply_err(reply);
	
	reply_msg(node_id, buf, sizeof(buf) );
}

void coord::reply_ok(node::node_id_t node_id) {
	char buf[sizeof(proto_coord::header_t)];
	proto_coord::header_t *reply = (proto_coord::header_t *) buf;
	
	proto_coord::reply_ok(reply);
	
	reply_msg(node_id, buf, sizeof(buf) );
}

void coord::reply_msg(node::node_id_t node_id, const char *buf, int buflen) {
	proto_coord::header_t *reply = (proto_coord::header_t *) buf;

	assert(buflen >= sizeof(proto_coord::header_t) );

	proto_coord::hton_hdr(reply);
	proto_base::send_udp_msg(m_socket, &m_nodes[node_id], buflen, buf);
}

int coord::add_edge(const edge &e) {
	pair<set<edge>::iterator,bool> ret;

	ret = m_edges.insert(e);

	return ret.second;
}

bool coord::network_ready() const {
	set<edge>::const_iterator it;

	for(it = m_edges.begin(); it != m_edges.end(); it++) {
		if( (m_nodes.find(it->n1) == m_nodes.end() ) || (m_nodes.find(it->n2) == m_nodes.end() ) ) {
			return false;
		}
	}

	return true;
}

/*int coord::add_edge(node::node_id_t n1, node::node_id_t n2) {
	node::node_id_t nodes[2] = {n1, n2};
	set<node::node_id_t> s(nodes, nodes+1);
	set<node::node_id_t>::const_iterator it;

	if(m_edges.find(s) == m_edges.end()) {
		return -1;	
	}
	else {
		m_edges.insert(s);
		return 0;
	}	
}
*/
