#include "node.h"

#include "net04_common.h"
#include "sock.h"
#include "proto_node.h"

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

void node::send_coord_fwd_ack(uint32_t msg_id, uint16_t type) const {
	int size = sizeof(proto_base::header_t) + 4;
	char buf[size];
	proto_base::header_t *hdr = (proto_base::header_t *) buf;
	
	if( type != proto_coord::TYPE_FWD_ERR && type != proto_coord::TYPE_FWD_ACK) {
		FATAL("invalid fwd ack type");
	}

	NET04_LOG("node %d: fwd ack %s for msg_id %d to coord\n", m_node_id, proto_coord::type_to_str(type), msg_id);

	hdr->id = m_node_id;
	hdr->type = type;
	hdr->msg_len = 4;
	
	*( (uint32_t *) (buf + sizeof(proto_base::header_t) ) ) = msg_id;

	send_coord_msg(buf, size);
}

void node::send_coord_msg(const char *buf, int buflen) const {
	proto_base::header_t *reply = (proto_base::header_t *) buf;

	assert(buflen >= sizeof(proto_base::header_t) );

	proto_base::hton_hdr(reply);
	proto_base::send_udp_msg(m_coord_socket, m_coord_addr, buflen, buf);
}

void node::request_coord_init() const {
	proto_base::header_t hdr;

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
	proto_base::header_t *hdr = (proto_base::header_t *) msg;	

	proto_base::ntoh_hdr(hdr);

	type = proto_base::msg_type(msg);
	len = proto_base::msg_len(msg);
	
	NET04_LOG("node %d: received %d byte '%s' (%d) message from coord\n", m_node_id, msglen, proto_coord::type_to_str(type), type); 

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

		case proto_coord::TYPE_SND_MSG : 
			on_send_message(msg, msglen);
			break;

		case proto_coord::TYPE_ERR : 
			printf("received TYPE_ERR from coord.");
			break;

		default:
			printf("unknown message type %d: ", type);
			print_hex_bytes(msg, msglen); printf("\n");
	}
}

void node::on_send_message(char *msg, int msglen) const {
	proto_base::header_t *hdr = (proto_base::header_t *) msg;
	node_id_t src, dest;
	
	proto_node::ntoh_msg_hdr((proto_node::msg_header_t *) msg + sizeof(proto_base::header_t) + sizeof(proto_node::msg_header_t) );

	src = proto_node::mhdr_src(msg);
	if(src != m_node_id) {
		FATAL("invalid source\n");
	}

	dest = proto_node::mhdr_dest(msg);
	if(dest == m_node_id) {
		on_message_receive(msg, msglen);
		send_coord_fwd_ack(proto_node::mhdr_msg_id(msg), proto_coord::TYPE_FWD_ACK);
		return;
	}

	hdr->id = m_node_id;
	hdr->type = proto_node::TYPE_SND_MSG;
	
	proto_node::mhdr_zero_route_list(msg);
	proto_node::mhdr_add_to_route_list(msg, m_node_id);

	send_coord_fwd_ack(proto_node::mhdr_msg_id(msg), proto_coord::TYPE_FWD_ERR);	
}

void node::on_message_receive(char *msg, int msglen) const {
	node_id_t id = proto_node::mhdr_src(msg);
	uint32_t msg_id = proto_node::mhdr_msg_id(msg);

	printf("node %d: received message %d from node %d: ", m_node_id, msg_id, id);
	proto_node::print_msg(msg, msglen);
	printf("\n");
}

void node::on_link_update(proto_base::header_t *hdr, uint16_t msg_len, int buflen, const char *buf) {
	proto_coord::link_desc_t *ld;
	link_map_t::iterator it;
	bool deleted = false;

	if(msg_len != sizeof(proto_coord::link_desc_t) ) {
		FATAL("unexpected size of link update message");
	}

	ld = (proto_coord::link_desc_t *) (buf + sizeof(proto_base::header_t));

	if(ld->cost == 0) {
		if( (it = m_links.find(ld->id) ) != m_links.end() ) {
			m_links.erase(it);
			deleted = true;
		}
		else {
			NET04_LOG("attempt to delete link to %d which does not exist\n", ld->id);
			return;
		}
	}
	else {
		m_links[ld->id].second = ld->cost;
		memset(&m_links[ld->id].first, 0, sizeof(struct sockaddr_in) );
		m_links[ld->id].first.sin_addr.s_addr = ld->s_addr;
		m_links[ld->id].first.sin_port = ld->port;
		m_links[ld->id].first.sin_family = AF_INET;
	}

	NET04_LOG("node %d: %s link to node %d", m_node_id, (deleted? "removed" : "added"), ld->id);
	if(!deleted) {
		NET04_LOG(" (%s:%d) with cost %d", inet_ntoa(m_links[ld->id].first.sin_addr), ntohs(m_links[ld->id].first.sin_port), ld->cost);
	}
	NET04_LOG("\n");

}

void node::on_request_table() const {	
	char *buf;
	proto_base::header_t *hdr;
	map<node_id_t, std::pair<struct sockaddr_in, cost_t> >::const_iterator link_it;
	map<node_id_t, fwd_entry_t >::const_iterator dv_it;
	proto_coord::table_info_t *tinfo;
	int entries = m_links.size() + m_dv_table.size();
	int bufsize = sizeof(proto_base::header_t) + entries*sizeof(proto_coord::table_info_t);
	
	if(entries < 1) {
		return;
	}

	buf = new char[bufsize];
	hdr = (proto_base::header_t *) buf;

	proto_coord::table_info(hdr, m_node_id, entries);
	
	tinfo = (proto_coord::table_info_t *) (buf + sizeof(proto_base::header_t) );

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

	NET04_LOG("node %d: send %d = %d + %d*%d byte table info data to coord\n", m_node_id, bufsize, sizeof(proto_base::header_t), entries, sizeof(proto_coord::table_info_t));
}

void node::listen_dv(void *instance) {
	node *n = (node *) instance;

	NET04_LOG("listen_dv: start\n");
	

	NET04_LOG("listen_dv: stop\n");
} 