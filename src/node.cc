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
	  m_pool(new thread_pool(2 + 8) ), m_registered(false) {

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
	time_t last_reg, tmp;
	double dif;
	char prefix[32];
	
	sprintf(prefix, "node %d", n->m_node_id);

	last_reg = 0;
 
	NET04_LOG("%s: listen_coord: start\n", prefix);
	set_nonblocking(n->m_coord_socket);

	while(1) {
		if( (n->m_registered != true) && ( (time(NULL) - last_reg) > 8) ) {
			NET04_LOG("%s: register with coord...\n", prefix);
			n->request_coord_init();
			last_reg = time(NULL);
		}
		else if(n->m_registered == true) {
			last_reg = time(NULL);
		}

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

void node::on_coord_msg(int msglen, char *msg) {
	uint16_t type, len;
	proto_coord::header_t *hdr = (proto_coord::header_t *) msg;	

	proto_coord::ntoh_hdr(hdr);

	type = proto_coord::msg_type(msg);
	len = proto_coord::msg_len(msg);
	
	NET04_LOG("node %d: received %d byte '%s' (%d) message from coord\n", m_node_id, msglen, proto_coord::type_to_str(type), type); //print_hex_bytes(msg, msglen); printf("\n");

	switch(type) {
		case proto_coord::TYPE_REG_ACK : 
			m_registered = true;
			break;

		case proto_coord::TYPE_NET_RST :
			m_dv_table.clear();
			m_links.clear();
			m_registered = false;
			break;

		case proto_coord::TYPE_LNK_UPD :
			on_link_update(hdr, len, msglen, msg);			
			break;

		case proto_coord::TYPE_REQ_TBL :
			on_request_table();			
			break;

		case proto_coord::TYPE_ERR : 
			printf("received TYPE_ERR from coord.");
			break;

		default:
			printf("unknown message type %d\n", type);
	}
}

void node::on_link_update(proto_coord::header_t *hdr, uint16_t msg_len, int buflen, const char *buf) {
	proto_coord::link_desc_t *ld;
	//map<node_id_t, struct sockaddr_in>::

	if(msg_len != sizeof(proto_coord::link_desc_t) ) {
		FATAL("unexpected size of link update message");
	}

	ld = (proto_coord::link_desc_t *) (buf + sizeof(proto_coord::header_t));

	m_links[ld->id].second = ld->cost;
	memset(&m_links[ld->id].first, 0, sizeof(struct sockaddr_in) );
	m_links[ld->id].first.sin_addr.s_addr = ld->s_addr;
	m_links[ld->id].first.sin_port = ld->port;
	m_links[ld->id].first.sin_family = AF_INET;
	
	NET04_LOG("node %d: added link to node %d (%s:%d) with cost %d\n", m_node_id, ld->id, inet_ntoa(m_links[ld->id].first.sin_addr), ntohs(m_links[ld->id].first.sin_port), ld->cost);
}

void node::on_request_table() const {	
	char *buf;
	proto_coord::header_t *hdr;
	map<node_id_t, std::pair<struct sockaddr_in, cost_t> >::const_iterator link_it;
	map<node_id_t, fwd_entry_t >::const_iterator dv_it;
	proto_coord::table_info_t *tinfo;
	int entries = m_links.size() + m_dv_table.size();
	int bufsize = sizeof(proto_coord::header_t) + entries*sizeof(proto_coord::table_info_t);
	
	if(entries < 1) {
		return;
	}

	buf = new char[bufsize];
	hdr = (proto_coord::header_t *) buf;

	proto_coord::table_info(hdr, m_node_id, entries);
	
	tinfo = (proto_coord::table_info_t *) (buf + sizeof(proto_coord::header_t) );

	for(link_it = m_links.begin(); link_it != m_links.end(); link_it++) {
		tinfo->dest = (*link_it).first;
		tinfo->link.id = (*link_it).first;
		tinfo->link.cost = (*link_it).second.second;
		tinfo++;
	}

	for(dv_it = m_dv_table.begin(); dv_it != m_dv_table.end(); dv_it++) {
		tinfo->dest = (*dv_it).first;
		tinfo->link.id = (*dv_it).second.id;
		tinfo->link.cost = (*dv_it).second.cost;
		tinfo++;
	}

	send_coord_msg(buf, bufsize);
	delete[] buf;

	NET04_LOG("node %d: send %d = %d + %d*%d byte table info data to coord\n", m_node_id, bufsize, sizeof(proto_coord::header_t), entries, sizeof(proto_coord::table_info_t));
}

void node::listen_dv(void *instance) {
	node *n = (node *) instance;

	NET04_LOG("listen_dv: start\n");
	

	NET04_LOG("listen_dv: stop\n");
} 