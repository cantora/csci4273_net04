#include "coord.h"

#include <cassert>

#include "net04_common.h"
#include "sock.h"
#include "proto_node.h"


using namespace net04;
using namespace net02;
using namespace net01;
using namespace std;

coord::coord(struct sockaddr_in *sin) : m_sin(sin), m_pool(new thread_pool(2 + 8) ), m_tbl_upd(false), 
		m_tbl_upd_msg(NULL), m_initialized_links(false), m_last_reg(time(NULL)), 
		m_send_wait(false), m_send_inc(0) {

	m_socket = sock::bound_udp_socket(m_sin);

	pthread_mutex_init(&m_tbl_upd_mtx, NULL);
	pthread_cond_init(&m_tbl_upd_cond, NULL);

	pthread_mutex_init(&m_send_mtx, NULL);
	pthread_cond_init(&m_send_cond, NULL);

	while(m_pool->dispatch_thread(listen_node, this, NULL) != 0) {
		usleep(1000);
	}

}

coord::~coord() {
	delete m_pool;

	pthread_mutex_destroy(&m_tbl_upd_mtx);
	pthread_cond_destroy(&m_tbl_upd_cond);
	
	pthread_mutex_destroy(&m_send_mtx);
	pthread_cond_destroy(&m_send_cond);
	
	close(m_socket);	
}

int coord::send(int timeout, node_id_t src, node_id_t dest, const char *msg, int msglen) {
	p_mutex_lock(&m_send_mtx);
	
	m_send_wait = true;

	send_message(src, dest, msg, msglen);

	/* wait to receive signal that we got a reply from node_id about its table */
	if(p_cond_timedwait_usec(&m_send_cond, &m_send_mtx, 1000000*timeout) == ETIMEDOUT) { /* wait for timeout secs */
		errno = ETIMEDOUT;
		return -1;
	}

	//NETO4_LOG("received signal\n");
	if(m_send_reply == NULL) {
		FATAL("m_send_reply is NULL");
	}
	
	print_fwd_reply_info();

	m_send_wait = false;
	delete[] m_send_reply;
	m_send_reply = NULL;
	m_send_inc++;

	p_mutex_unlock(&m_send_mtx);
}

void coord::print_fwd_reply_info() const {
	node_id_t id = proto_base::msg_node_id(m_send_reply);
	uint16_t type = proto_base::msg_type(m_send_reply);

	printf("reply from node %d: %s (%d)\n", id, proto_coord::type_to_str(type), type);	
}

void coord::send_message(node_id_t src, node_id_t dest, const char *msg, int msglen) const {
	proto_base::header_t *hdr;
	int msgsize;
	char *buf;
	char *n_offset;
	proto_node::msg_header_t *mhdr;

	msgsize = sizeof(proto_base::header_t) + sizeof(proto_node::msg_header_t) + msglen;
	buf = new char[msgsize];
	hdr = (proto_base::header_t *) buf;

	proto_coord::send_msg(hdr, msglen);

	n_offset = buf + sizeof(proto_base::header_t);
	mhdr = (proto_node::msg_header_t *) n_offset;
	mhdr->msg_id = m_send_inc;
	mhdr->src = src;
	mhdr->dest = dest;
	memset(mhdr->route, 0, proto_node::max_route_size);

	memcpy(n_offset + sizeof(proto_node::msg_header_t), msg, msglen);
	
	//print_hex_bytes(buf, msgsize); printf("\n");
	send_msg_to_node(src, buf, msgsize );

	delete[] buf;
}

int coord::link_cost_change(node_id_t n1, node_id_t n2, cost_t cost) {
	edge e(n1, n2);
	map<edge, cost_t>::iterator it;

	it = m_edges.find(e);
	
	if(it == m_edges.end() ) {
		return -1;
	}

	//(it)->cost = e.cost;
	m_edges.erase(it);
	m_edges[e] = cost;

	send_cost_change(&e, cost, e.first);
	send_cost_change(&e, cost, e.second);
}

void coord::print_all_tables() {
	node_map_t::const_iterator it;
	
	for(it = m_nodes.begin(); it != m_nodes.end(); it++) {
		printf("node %d:\n", (*it).first);
		print_table((*it).first);
	}

}

void coord::print_table(node_id_t node_id) {
	//int status;

	p_mutex_lock(&m_tbl_upd_mtx);
	
	/*status = pthread_mutex_trylock(&m_tbl_upd_mtx);
	
	if(status == EBUSY) {
		FATAL("tbl_upd_mtx is already locked");
	}
	else if(status != 0) {
		FATAL("mutex trylock error");
	}*/
	
	m_tbl_upd = true;

	request_table(node_id);

	/* wait to receive signal that we got a reply from node_id about its table */
	p_cond_wait(&m_tbl_upd_cond, &m_tbl_upd_mtx);
	
	//NETO4_LOG("received signal\n");
	/* now we should have a non-null m_tbl_upd_msg */
	if(m_tbl_upd_msg == NULL) {
		FATAL("m_tbl_upd_msg is NULL");
	}
	
	//NET04_LOG("got table update reply from node %d\n", node_id);

	print_tbl_upd_msg();

	m_tbl_upd = false;
	delete[] m_tbl_upd_msg;
	m_tbl_upd_msg = NULL;
	p_mutex_unlock(&m_tbl_upd_mtx);
}

void coord::print_tbl_upd_msg() const {
	proto_base::header_t *hdr;
	proto_coord::table_info_t *tinfo;
	int tinfo_arr_bytes, i, entries;

	tinfo_arr_bytes = m_tbl_upd_msg_len - sizeof(proto_base::header_t);
	entries = tinfo_arr_bytes/sizeof(proto_coord::table_info_t);

	assert( (tinfo_arr_bytes % sizeof(proto_coord::table_info_t) ) == 0);

	tinfo = (proto_coord::table_info_t *) (m_tbl_upd_msg + sizeof(proto_base::header_t) );

	printf("\t+-dest\t+link\t+cost\n");
	printf("\t+---------------------\n");
	for(i = 0; i < entries; i++) {
		//if(tinfo[i].dest == tinfo[i].link.id) printf("*");
		printf("\t+%d\t+%d\t+%d\n", tinfo[i].dest, tinfo[i].link.id, tinfo[i].link.cost);
	}	
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

	sleep(3);

	while(1) {
		if( (c->m_initialized_links == false) && c->network_ready() ) {
			NET04_LOG("initialize links with nodes\n");
			c->send_links();
		}
		if( ((time(NULL) - c->m_last_reg) > 8) && !c->network_ready() ) {
			NET04_LOG("waited too long for nodes, send reset...\n");
			c->bcast_reset();
			c->m_last_reg = time(NULL);
		}

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

		fflush(stdout);
	}

	assert(false); // shouldnt get here
}

void coord::on_node_msg(int msglen, char *msg, const struct sockaddr_in *sin) {
	uint16_t type;
	node_id_t node_id;
	proto_base::header_t *hdr = (proto_base::header_t *) msg;	
	char dbg_msg[256];

	assert(sin != NULL);

	proto_base::ntoh_hdr(hdr);

	type = proto_base::msg_type(msg);
	node_id = proto_base::msg_node_id(msg);

	sprintf(dbg_msg, "received %d byte '%s' message from %s:%d (%d). ", msglen, proto_coord::type_to_str(type), inet_ntoa(sin->sin_addr), ntohs(sin->sin_port), node_id);

	switch(type) {
		case proto_coord::TYPE_REQ_INIT :
			NET04_LOG(dbg_msg);
			on_req_init(node_id, msg, sin);
			break;
		case proto_coord::TYPE_TBL_INFO :
			on_tbl_info(node_id, msg, msglen);
			break;
		case proto_coord::TYPE_FWD_ERR :
		case proto_coord::TYPE_FWD_ACK :
			on_fwd_msg(type, node_id, msg, msglen);
			break;
		default: 
			NET04_LOG(dbg_msg);
			printf("unknown message type %d\n", type);
	}

	fflush(stdout);
	
}

void coord::on_req_init(node_id_t node_id, const char *msg, const struct sockaddr_in *sin) {
	proto_coord::link_desc_t *ld;

	if(m_nodes.count(node_id) > 0) {
		NET04_LOG("%d already registered, reply with error.\n", node_id);
		reply_err(node_id, sin);
		
		return;
	}
	else {
		ld = (proto_coord::link_desc_t *) (msg + sizeof(proto_base::header_t) );
		memset(&m_nodes[node_id].coord_sin, 0, sizeof(struct sockaddr_in));
		memset(&m_nodes[node_id].dv_sin, 0, sizeof(struct sockaddr_in));

		m_nodes[node_id].coord_sin = *sin;
		
		m_nodes[node_id].dv_sin.sin_addr.s_addr = ld->s_addr;
		m_nodes[node_id].dv_sin.sin_port = ld->port;
		m_nodes[node_id].dv_sin.sin_family = AF_INET;
	
		if(ld->id != node_id) {
			FATAL("got contradictory registration info");
		}

		NET04_LOG("reply reg_ack (network size: %d)\n", m_nodes.size());
		reply_reg_ack(node_id);
		//send_table(node_id);
		m_last_reg = time(NULL);
	}
}

void coord::on_fwd_msg(uint16_t type, node_id_t, char *buf, int buflen) {
	int status;
	uint32_t msg_id = proto_node::mhdr_msg_id(buf);
	
	p_mutex_lock(&m_send_mtx);
	
	if(m_send_wait != true || msg_id != m_send_inc) {
		NET04_LOG("not waiting for fwd_msg data on message %d, ignoring...\n", msg_id);
		return;
	}
	
	m_send_reply_len = buflen;
	m_send_reply = new char[buflen];
	
	memcpy(m_send_reply, buf, buflen);
	
	//NET04_LOG("signal that table info data is ready\n");
	p_cond_signal(&m_send_cond);
	p_mutex_unlock(&m_send_mtx);
}

void coord::on_tbl_info(node_id_t, const char *buf, int buflen) {
	int status;
	
	p_mutex_lock(&m_tbl_upd_mtx);
	
	if(m_tbl_upd != true) {
		NET04_LOG("not waiting for table info data, ignoring...\n");
		return;
	}
	
	m_tbl_upd_msg_len = buflen;
	m_tbl_upd_msg = new char[buflen];
	
	memcpy(m_tbl_upd_msg, buf, buflen);
	
	//NET04_LOG("signal that table info data is ready\n");
	p_cond_signal(&m_tbl_upd_cond);
	p_mutex_unlock(&m_tbl_upd_mtx);
}

void coord::send_links() {
	node_map_t::const_iterator it;
	
	for(it = m_nodes.begin(); it != m_nodes.end(); it++) {
		send_table((*it).first);
	}	

	m_initialized_links = true;
}

void coord::send_table(node_id_t node_id) const {
	map<edge, cost_t>::const_iterator it;

	for(it = m_edges.begin(); it != m_edges.end(); it++) {
		if((*it).first.contains(node_id) ) {
			send_cost_change(&(*it).first, (*it).second, node_id);
			usleep(1000);
		}
	}
}

void coord::request_table(node_id_t node_id) const {
	char buf[sizeof(proto_base::header_t)];
	proto_base::header_t *hdr = (proto_base::header_t *) buf;
	
	proto_coord::request_table(hdr);
		
	send_msg_to_node(node_id, buf, sizeof(buf) );
}

void coord::send_cost_change(const edge *e, cost_t cost, node_id_t node_id) const {
	char buf[sizeof(proto_base::header_t) + sizeof(proto_coord::link_desc_t)];
	proto_base::header_t *hdr = (proto_base::header_t *) buf;
	proto_coord::link_desc_t ld;
	node_map_t::const_iterator it;
	const struct sockaddr_in *sin;
	
	NET04_LOG("send cost change to node %d: {:n1 => %d, :n2 => %d, :cost => %d}\n", node_id, e->first, e->second, cost);

	if(e->first == node_id) {
		ld.id = e->second;
	}
	else if(e->second == node_id) {
		ld.id = e->first;
	}
	else {
		FATAL("edge doesnt contain node_id");
	}

	if(ld.id == node_id) {
		FATAL("node_id is the same as ld.id");
	}

	ld.cost = cost;
	
	it = m_nodes.find(ld.id);

	if(it == m_nodes.end() ) {
		FATAL("could not find node_id");
	}

	sin = &(*it).second.dv_sin;
	/* sockaddr_in already in network byte order */
	ld.s_addr = sin->sin_addr.s_addr; 
	ld.port = sin->sin_port;

	proto_coord::link_update(hdr);
	
	*( (proto_coord::link_desc_t *)(buf + sizeof(proto_base::header_t)) ) = (proto_coord::link_desc_t) ld;
	
	send_msg_to_node(node_id, buf, sizeof(buf) );
}

void coord::bcast_reset() {
	char buf[sizeof(proto_base::header_t)];
	proto_base::header_t *hdr = (proto_base::header_t *) buf;
	
	proto_coord::net_reset(hdr);
		
	bcast_msg(buf, sizeof(buf) );

	m_nodes.clear();
	m_initialized_links = false;
}

void coord::bcast_msg(char *buf, int buflen) const {
	node_map_t::const_iterator it;
	proto_base::header_t *hdr = (proto_base::header_t *) buf;
	
	assert(buflen >= sizeof(proto_base::header_t) );
	
	proto_base::hton_hdr(hdr);
	for(it = m_nodes.begin(); it != m_nodes.end(); it++) {
		proto_base::send_udp_msg(m_socket, &(*it).second.coord_sin, buflen, buf);
		usleep(10000);
	}
}

void coord::reply_net_size(node_id_t node_id) const {
	char buf[sizeof(proto_base::header_t) + sizeof(proto_coord::net_size_t)];
	proto_base::header_t *reply = (proto_base::header_t *) buf;
	
	proto_coord::reply_net_size(reply);
	
	assert(sizeof(proto_coord::net_size_t) == 1);
	*(buf + sizeof(proto_base::header_t) ) = (proto_coord::net_size_t) m_nodes.size();
	
	send_msg_to_node(node_id, buf, sizeof(buf) );
}

void coord::reply_err(node_id_t node_id, const struct sockaddr_in *sin) const {
	char buf[sizeof(proto_base::header_t)];
	proto_base::header_t *reply = (proto_base::header_t *) buf;
	
	proto_coord::reply_err(reply);
	
	send_msg(buf, sizeof(buf), sin );
}

void coord::reply_ok(node_id_t node_id) const {
	char buf[sizeof(proto_base::header_t)];
	proto_base::header_t *reply = (proto_base::header_t *) buf;
	
	proto_coord::reply_ok(reply);
	
	send_msg_to_node(node_id, buf, sizeof(buf) );
}

void coord::reply_reg_ack(node_id_t node_id) const {
	char buf[sizeof(proto_base::header_t)];
	proto_base::header_t *reply = (proto_base::header_t *) buf;
	
	proto_coord::reply_reg_ack(reply);
	
	send_msg_to_node(node_id, buf, sizeof(buf) );
}

void coord::send_msg_to_node(node_id_t node_id, char *buf, int buflen) const {
	const struct sockaddr_in *sin;
	node_map_t::const_iterator it;

	it = m_nodes.find(node_id);
	
	if(it == m_nodes.end() ) {
		FATAL("could not find node");
	}

	sin = &(*it).second.coord_sin;

	send_msg(buf, buflen, sin);
}

void coord::send_msg(char *buf, int buflen, const struct sockaddr_in *sin) const {
	proto_base::header_t *hdr = (proto_base::header_t *) buf;
	
	assert(buflen >= sizeof(proto_base::header_t) );
	
	proto_base::hton_hdr(hdr);
	proto_base::send_udp_msg(m_socket, sin, buflen, buf);
}

bool coord::add_edge(const edge &e, cost_t cost) {
	pair<map<edge, cost_t>::iterator,bool> ret;

	ret = m_edges.insert(make_pair(e, cost) );

	return ret.second;
}

bool coord::network_ready() const {
	map<edge, cost_t>::const_iterator it;

	for(it = m_edges.begin(); it != m_edges.end(); it++) {
		if( (m_nodes.find(it->first.first) == m_nodes.end() ) || (m_nodes.find(it->first.second) == m_nodes.end() ) ) {
			return false;
		}
	}

	return true;
}
