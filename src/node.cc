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
	send_msg(m_coord_socket, m_coord_addr, buf, buflen);
}

void node::send_msg(int socket, const struct sockaddr_in *sin, const char *buf, int buflen) const {
	proto_base::header_t *hdr = (proto_base::header_t *) buf;

	assert(buflen >= sizeof(proto_base::header_t) );

	proto_base::hton_hdr(hdr);
	proto_base::send_udp_msg(socket, sin, buflen, buf);
}

void node::request_coord_init() const {
	int size = sizeof(proto_base::header_t) + sizeof(proto_coord::link_desc_t);
	char buf[size];
	proto_base::header_t *hdr = (proto_base::header_t *) buf;
	proto_coord::link_desc_t *ld = (proto_coord::link_desc_t *) (buf + sizeof(proto_base::header_t) );

	proto_coord::request_coord_init(hdr, m_node_id);
	ld->s_addr = 0; /* coord should use the ip from request */ //m_dv_sin->sin_addr.s_addr;
	ld->port = m_dv_sin->sin_port;
	ld->id = m_node_id;
	ld->cost = 0;

	//NET04_LOG("reg addr %s:%d\n", inet_ntoa(m_dv_sin->sin_addr), ntohs(m_dv_sin->sin_port));

	send_coord_msg(buf, size);
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
	node_id_t src;
	
	proto_node::ntoh_msg_hdr((proto_node::msg_header_t *) msg + sizeof(proto_base::header_t) );

	src = proto_node::mhdr_src(msg);
	if(src != m_node_id) {
		FATAL("invalid source\n");
	}

	proto_node::mhdr_zero_route_list(msg);
	
	forward(msg, msglen);
}

void node::forward(char *msg, int msglen) const {
	proto_base::header_t *hdr = (proto_base::header_t *) msg;
	node_id_t dest, nh;

	dest = proto_node::mhdr_dest(msg);	
	//src = proto_node::mhdr_src(msg);
	if(dest == m_node_id) {
		on_message_receive(msg, msglen);
		send_coord_fwd_ack(proto_node::mhdr_msg_id(msg), proto_coord::TYPE_FWD_ACK);
		return;
	}

	hdr->id = m_node_id;
	hdr->type = proto_node::TYPE_FWD_MSG;
	
	proto_node::mhdr_add_to_route_list(msg, m_node_id);

	nh = next_hop(dest);

	if(nh == 0) {
		NET04_LOG("node %d: could not find route to node %d\n", m_node_id, dest);
		send_coord_fwd_ack(proto_node::mhdr_msg_id(msg), proto_coord::TYPE_FWD_ERR);
		return;
	}

	proto_node::hton_msg_hdr((proto_node::msg_header_t *) msg + sizeof(proto_base::header_t) );
	
	NET04_LOG("node %d: forward message %d to node %d for delivery to %d\n", m_node_id, proto_node::mhdr_msg_id(msg), nh, dest);

	send_link_msg(nh, msg, msglen);
}

node_id_t node::next_hop(node_id_t dest) const {
	map<node_id_t, fwd_entry_t>::const_iterator dv_it;
	link_map_t::const_iterator link_it;

	link_it = m_links.find(dest);
	
	if(link_it != m_links.end() ) {
		return link_it->first;
	}

	dv_it = m_dv_table.find(dest);

	if(dv_it == m_dv_table.end() ) {
		return 0;
	}

	return dv_it->second.id;
}

void node::send_link_msg(node_id_t link, const char *buf, int buflen) const {
	link_map_t::const_iterator it;

	it = m_links.find(link);

	if(it == m_links.end() ) {
		FATAL("link does not exist");
	}
	
	send_msg(m_dv_socket, &it->second.first, buf, buflen);
}

void node::on_message_receive(const char *msg, int msglen) const {
	node_id_t id = proto_node::mhdr_src(msg);
	uint32_t msg_id = proto_node::mhdr_msg_id(msg);
	//uint16_t len = proto_base::msg_len(msg); //((proto_base::header_t *) msg)->msg_len;

	printf("node %d: received message %d from node %d: ", m_node_id, msg_id, id);
	proto_node::print_msg(msg);
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
	int msglen;
	char msgbuf[proto_node::max_msg_len];
	time_t last_route_bcast, tmp;
	char prefix[32];
	struct sockaddr_in sin;
	socklen_t sinlen = sizeof(struct sockaddr_in);

	sprintf(prefix, "node %d DV", n->m_node_id);
 
	NET04_LOG("%s: listen_node: start\n", prefix);
	set_nonblocking(n->m_dv_socket);

	last_route_bcast = 0;

	while(1) {
		if(n->m_links.size() > 0) {
			if(last_route_bcast == 0) last_route_bcast = time(NULL);

			if(time(NULL) - last_route_bcast > 5) {
				NET04_LOG("%s: broadcast routes to neighbors...\n", prefix);
				n->route_bcast();
				last_route_bcast = time(NULL);
			}
		}
	
		msglen = recvfrom(n->m_dv_socket, msgbuf, sizeof(msgbuf), 0, (sockaddr *) &sin, &sinlen);
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

		n->on_node_msg(msglen, msgbuf, &sin);
	}
	
	FATAL("shouldnt get here");
	//NET04_LOG("listen_dv: stop\n");
} 

void node::on_node_msg(int msglen, char *msg, const struct sockaddr_in *sin) {
	uint16_t type, len;
	proto_base::header_t *hdr = (proto_base::header_t *) msg;	
	node_id_t node_id;
	char debug_log[256];

	proto_base::ntoh_hdr(hdr);

	type = proto_base::msg_type(msg);
	len = proto_base::msg_len(msg);
	node_id = proto_base::msg_node_id(msg);

	sprintf(debug_log, "node %d DV: received %d byte '%s' (%d) message from node %d (%s:%d)\n", m_node_id, msglen, proto_node::type_to_str(type), type, node_id, inet_ntoa(sin->sin_addr), ntohs(sin->sin_port));
	

	switch(type) {
		case proto_node::TYPE_FWD_MSG :
			NET04_LOG(debug_log);
			on_fwd_message(msg, msglen);
			break;

		case proto_node::TYPE_ROUTE_INFO :
			NET04_LOG("*");//NET04_LOG(debug_log);
			on_route_info(msg, msglen);
			break;

		default:
			NET04_LOG(debug_log);
			printf("unknown message type %d: ", type);
			print_hex_bytes(msg, msglen); printf("\n");
	}
}

void node::on_fwd_message(char *msg, int msglen) const {
	proto_node::ntoh_msg_hdr((proto_node::msg_header_t *) msg + sizeof(proto_base::header_t) );
	
	forward(msg, msglen);
}

void node::on_route_info(char *msg, int msglen) {
	proto_base::header_t *hdr = (proto_base::header_t *) msg;
	fwd_entry_t *entry = (fwd_entry_t *) (msg + sizeof(proto_base::header_t) );
	node_id_t from;
	link_map_t::const_iterator link_it, from_link;
	dv_map_t::const_iterator dv_it;
	cost_t aggr_cost;

	from = proto_base::msg_node_id(msg);
	
	if( (from_link = m_links.find(from) ) == m_links.end() ) {
		FATAL("why am i getting a fwd entry from a node i dont have a link with?");
	}

	aggr_cost = from_link->second.second + entry->cost;
	//NET04_LOG("\tnew route info: on %d --(%d + %d)--> %d\n", from, from_link->second.second, entry->cost, entry->id);
		
	if( (link_it = m_links.find(entry->id) ) != m_links.end() ) {
		if(link_it->second.second <= (aggr_cost) ) { 
			//NET04_LOG("\talready have a link with lower cost to node %d. ignoring route info\n", entry->id);
			return;
		} // else keep going to see if we have a better route already in our table
	}

	if( (dv_it = m_dv_table.find(entry->id) ) != m_dv_table.end() ) {
		if(dv_it->second.cost <= (aggr_cost) ) {
			//NET04_LOG("\talready have a route with lower cost to node %d. ignoring route info\n", entry->id);
			return;
		} // else keep going and store this route
	}

	/* we dont have a route to this node yet, so store this route */
	m_dv_table[entry->id].cost = aggr_cost;
	m_dv_table[entry->id].id = from;
	
	NET04_LOG("\tstored route info to %d at cost %d on link %d...\n", entry->id, aggr_cost, from);	
}

void node::route_bcast() const {
	link_map_t::const_iterator it;

	for(it = m_links.begin(); it != m_links.end(); it++) {
		send_routes(it->first);
	}
}

void node::send_routes(node_id_t node_id) const {
	dv_map_t::const_iterator it;
	link_map_t::const_iterator link_it;
	fwd_entry_t route;

	for(link_it = m_links.begin(); link_it != m_links.end(); link_it++) {
		if(link_it->first == node_id) { /* dont tell a node about my link to it :) */
			continue;
		}

		route.id = link_it->first;
		route.cost = link_it->second.second;

		send_route(node_id, &route);
	}

	for(it = m_dv_table.begin(); it != m_dv_table.end(); it++) {
		if(it->second.id == node_id) { /* dont tell this node about routes it gives me */
			continue;
		}
		
		route.id = it->first;
		route.cost = it->second.cost;

		send_route(node_id, &route);
	}
}

void node::send_route(node_id_t node_id, const fwd_entry_t *route) const {
	int size = sizeof(proto_base::header_t) + sizeof(fwd_entry_t);
	char buf[size];
	proto_base::header_t *hdr = (proto_base::header_t *) buf;
	fwd_entry_t *r = (fwd_entry_t *) (buf + sizeof(proto_base::header_t) );

	proto_node::route_info(hdr, m_node_id);

	r->id = route->id;
	r->cost = route->cost;

	send_link_msg(node_id, buf, size);
}