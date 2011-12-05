#ifndef COORD_H
#define COORD_H

#include <cerrno>
#include <string>
#include <map>
#include <cstdio>
#include <set>

extern "C" {
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netdb.h> 
#include <arpa/inet.h>
}

#include "node.h"
#include "thread_pool.h"
#include "proto_coord.h"
#include "proto_base.h"
#include "uo_pair.h"

namespace net04 {

class coord {
	public:
		typedef uo_pair<node_id_t> edge;

		coord(struct sockaddr_in *sin);
		~coord();

		static void listen_node(void *instance);
		
		unsigned short port() const {
			return ntohs(m_sin->sin_port);
		}

		unsigned short nodes() const {
			return m_nodes.size();
		}

		unsigned short edges() const {
			return m_edges.size();
		}

		bool add_edge(const edge &e, cost_t cost);
		
		bool network_ready() const;
	
		void bcast_reset();

		int send(int timeout, node_id_t src, node_id_t dest, const char *msg, int msglen);
		int link_cost_change(node_id_t n1, node_id_t n2, cost_t cost);

		void print_all_tables();
		void print_table(node_id_t node_id);

	private:

		void on_node_msg(int msglen, char *msg, const struct sockaddr_in *sin);
		
		void on_req_init(node_id_t node_id, const char *msg, const struct sockaddr_in *sin);
		void on_tbl_info(node_id_t, const char *buf, int buflen);
		void on_fwd_msg(uint16_t type, node_id_t, char *buf, int buflen);

		void print_tbl_upd_msg() const;
		void print_fwd_reply_info() const;

		void send_links();
		void send_table(node_id_t node_id) const;
		void send_cost_change(const edge *e, cost_t cost, node_id_t node_id) const;
		void send_message(node_id_t src, node_id_t dest, const char *msg, int msglen) const;

		void request_table(node_id_t node_id) const;
		void reply_net_size(node_id_t node_id) const;
		void reply_ok(node_id_t node_id) const;
		void reply_err(node_id_t node_id, const struct sockaddr_in *sin) const;
		void reply_reg_ack(node_id_t node_id) const;
		void send_msg_to_node(node_id_t node_id, char *buf, int buflen) const;
		void send_msg(char *buf, int buflen, const struct sockaddr_in *sin) const;
		void bcast_msg(char *buf, int buflen) const;

		int m_socket;
		struct sockaddr_in *const m_sin;

		net02::thread_pool *const m_pool;

		struct node_addr_t {
			struct sockaddr_in coord_sin;
			struct sockaddr_in dv_sin;
		};

		typedef std::map<node_id_t, node_addr_t> node_map_t;
		node_map_t m_nodes;

		//typedef std::set<edge, bool(*)(const edge &, const edge &)> edge_set_t;

		//std::set<edge> m_edges;
		std::map<edge, cost_t> m_edges;

		pthread_mutex_t m_tbl_upd_mtx;
		pthread_cond_t m_tbl_upd_cond;
		bool m_tbl_upd;
		char *m_tbl_upd_msg;
		int m_tbl_upd_msg_len;

		pthread_mutex_t m_send_mtx;
		pthread_cond_t m_send_cond;
		bool m_send_wait;
		uint32_t m_send_inc;
		char *m_send_reply;
		int m_send_reply_len;
		

		bool m_initialized_links;
		time_t m_last_reg;

}; /* coord */

}; /* net04 */
#endif /* COORD_H */
