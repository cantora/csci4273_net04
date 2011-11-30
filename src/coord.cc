#include "coord.h"

#include <cassert>

#include "net04_common.h"
#include "sock.h"

using namespace net04;
using namespace net02;
using namespace net01;
using namespace std;

coord::coord(struct sockaddr_in *sin) : m_sin(sin), m_pool(new thread_pool(2 + 8) ), m_tbl_upd(false), m_tbl_upd_msg(NULL) {
	m_socket = sock::bound_udp_socket(m_sin);

	pthread_mutex_init(&m_tbl_upd_mtx, NULL);
	pthread_cond_init(&m_tbl_upd_cond, NULL);

	while(m_pool->dispatch_thread(listen_node, this, NULL) != 0) {
		usleep(1000);
	}

}

coord::~coord() {
	delete m_pool;

	pthread_mutex_destroy(&m_tbl_upd_mtx);
	pthread_cond_destroy(&m_tbl_upd_cond);
	
	close(m_socket);	
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
	
	NET04_LOG("got table update reply from node %d\n", node_id);

	print_tbl_upd_msg();

	m_tbl_upd = false;
	delete[] m_tbl_upd_msg;
	m_tbl_upd_msg = NULL;
	p_mutex_unlock(&m_tbl_upd_mtx);
}

void coord::print_tbl_upd_msg() const {
	proto_coord::header_t *hdr;
	proto_coord::table_info_t *tinfo;
	int tinfo_arr_bytes, i, entries;

	tinfo_arr_bytes = m_tbl_upd_msg_len - sizeof(proto_coord::header_t);
	entries = tinfo_arr_bytes/sizeof(proto_coord::table_info_t);

	assert( (tinfo_arr_bytes % sizeof(proto_coord::table_info_t) ) == 0);

	tinfo = (proto_coord::table_info_t *) (m_tbl_upd_msg + sizeof(proto_coord::header_t) );

	for(i = 0; i < entries; i++) {
		//if(tinfo[i].dest == tinfo[i].link.id) printf("*");
		printf("node %d --(%d)--> %d link\n", tinfo[i].dest, tinfo[i].link.cost, tinfo[i].link.id);
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
	node_id_t node_id;
	proto_coord::header_t *hdr = (proto_coord::header_t *) msg;	

	assert(sin != NULL);

	proto_coord::ntoh_hdr(hdr);

	type = proto_coord::msg_type(msg);
	node_id = proto_coord::msg_node_id(msg);

	NET04_LOG("received %d byte '%s' message from %s:%d (%d). ", msglen, proto_coord::type_to_str(type), inet_ntoa(sin->sin_addr), ntohs(sin->sin_port), node_id);

	switch(type) {
		case proto_coord::TYPE_REQ_INIT :
			on_req_init(node_id, sin);
			break;
		case proto_coord::TYPE_TBL_INFO :
			on_tbl_info(node_id, msg, msglen);
			break;
		default: 
			printf("unknown message type %d\n", type);
	}

	fflush(stdout);
	
}

void coord::on_req_init(node_id_t node_id, const struct sockaddr_in *sin) {

	if(m_nodes.count(node_id) > 0) {
		NET04_LOG("%d already registered, reply with error.\n", node_id);
		reply_err(node_id, sin);
		
		return;
	}
	else {
		m_nodes[node_id] = *sin;

		NET04_LOG("reply reg_ack (network size: %d)\n", m_nodes.size());
		reply_reg_ack(node_id);
		send_table(node_id);
	}
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
	
	NET04_LOG("signal that table info data is ready\n");
	p_cond_signal(&m_tbl_upd_cond);
	p_mutex_unlock(&m_tbl_upd_mtx);
}

void coord::send_table(node_id_t node_id) const {
	set<edge>::const_iterator it;

	for(it = m_edges.begin(); it != m_edges.end(); it++) {
		if((*it).contains(node_id) ) {
			send_cost_change(&(*it), node_id);
		}
	}
}

void coord::request_table(node_id_t node_id) const {
	char buf[sizeof(proto_coord::header_t)];
	proto_coord::header_t *hdr = (proto_coord::header_t *) buf;
	
	proto_coord::request_table(hdr);
		
	send_msg_to_node(node_id, buf, sizeof(buf) );
}

void coord::send_cost_change(const edge *e, node_id_t node_id) const {
	char buf[sizeof(proto_coord::header_t) + sizeof(proto_coord::link_desc_t)];
	proto_coord::header_t *hdr = (proto_coord::header_t *) buf;
	proto_coord::link_desc_t ld;
	map<node_id_t, struct sockaddr_in>::const_iterator it;
	const struct sockaddr_in *sin;

	NET04_LOG("send cost change to node %d: {:n1 => %d, :n2 => %d, :cost => %d}\n", node_id, e->n1, e->n2, e->cost);

	if(e->n1 == node_id) {
		ld.id = e->n2;
	}
	else if(e->n2 == node_id) {
		ld.id = e->n1;
	}
	else {
		FATAL("edge doesnt contain node_id");
	}

	if(ld.id == node_id) {
		FATAL("node_id is the same as ld.id");
	}

	ld.cost = e->cost;
	
	it = m_nodes.find(node_id);

	if(it == m_nodes.end() ) {
		FATAL("could not find node_id");
	}

	sin = &(*it).second;
	/* sockaddr_in already in network byte order */
	ld.s_addr = sin->sin_addr.s_addr; 
	ld.port = sin->sin_port;

	proto_coord::link_update(hdr);
	
	*( (proto_coord::link_desc_t *)(buf + sizeof(proto_coord::header_t)) ) = (proto_coord::link_desc_t) ld;
	
	send_msg_to_node(node_id, buf, sizeof(buf) );
}

void coord::bcast_reset() {
	char buf[sizeof(proto_coord::header_t)];
	proto_coord::header_t *hdr = (proto_coord::header_t *) buf;
	
	proto_coord::net_reset(hdr);
		
	bcast_msg(buf, sizeof(buf) );

	m_nodes.clear();
}

void coord::bcast_msg(char *buf, int buflen) const {
	map<node_id_t, struct sockaddr_in>::const_iterator it;
	proto_coord::header_t *hdr = (proto_coord::header_t *) buf;
	
	assert(buflen >= sizeof(proto_coord::header_t) );
	
	proto_coord::hton_hdr(hdr);
	for(it = m_nodes.begin(); it != m_nodes.end(); it++) {
		proto_base::send_udp_msg(m_socket, &(*it).second, buflen, buf);
	}
}

void coord::reply_net_size(node_id_t node_id) const {
	char buf[sizeof(proto_coord::header_t) + sizeof(proto_coord::net_size_t)];
	proto_coord::header_t *reply = (proto_coord::header_t *) buf;
	
	proto_coord::reply_net_size(reply);
	
	assert(sizeof(proto_coord::net_size_t) == 1);
	*(buf + sizeof(proto_coord::header_t) ) = (proto_coord::net_size_t) m_nodes.size();
	
	send_msg_to_node(node_id, buf, sizeof(buf) );
}

void coord::reply_err(node_id_t node_id, const struct sockaddr_in *sin) const {
	char buf[sizeof(proto_coord::header_t)];
	proto_coord::header_t *reply = (proto_coord::header_t *) buf;
	
	proto_coord::reply_err(reply);
	
	send_msg(buf, sizeof(buf), sin );
}

void coord::reply_ok(node_id_t node_id) const {
	char buf[sizeof(proto_coord::header_t)];
	proto_coord::header_t *reply = (proto_coord::header_t *) buf;
	
	proto_coord::reply_ok(reply);
	
	send_msg_to_node(node_id, buf, sizeof(buf) );
}

void coord::reply_reg_ack(node_id_t node_id) const {
	char buf[sizeof(proto_coord::header_t)];
	proto_coord::header_t *reply = (proto_coord::header_t *) buf;
	
	proto_coord::reply_reg_ack(reply);
	
	send_msg_to_node(node_id, buf, sizeof(buf) );
}

void coord::send_msg_to_node(node_id_t node_id, char *buf, int buflen) const {
	const struct sockaddr_in *sin;
	map<node_id_t, struct sockaddr_in>::const_iterator it;

	it = m_nodes.find(node_id);
	
	if(it == m_nodes.end() ) {
		FATAL("could not find node");
	}

	sin = &(*it).second;

	send_msg(buf, buflen, sin);
}

void coord::send_msg(char *buf, int buflen, const struct sockaddr_in *sin) const {
	proto_coord::header_t *hdr = (proto_coord::header_t *) buf;
	
	assert(buflen >= sizeof(proto_coord::header_t) );
	
	proto_coord::hton_hdr(hdr);
	proto_base::send_udp_msg(m_socket, sin, buflen, buf);
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

/*int coord::add_edge(node_id_t n1, node_id_t n2) {
	node_id_t nodes[2] = {n1, n2};
	set<node_id_t> s(nodes, nodes+1);
	set<node_id_t>::const_iterator it;

	if(m_edges.find(s) == m_edges.end()) {
		return -1;	
	}
	else {
		m_edges.insert(s);
		return 0;
	}	
}
*/
