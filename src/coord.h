#ifndef COORD_H
#define COORD_H

#include <cerrno>
#include <string>
#include <map>

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


namespace net04 {

class coord {
	public:

		class edge {
			public:
			node_id_t n1;
			node_id_t n2;
			cost_t cost;

			/*void serialize(char *buf) {
				assert(sizeof(node_id_t) == 1);
				assert(sizeof(cost_t) == 1);

				*(buf) = n1;
				*(buf + 1) = n2;
				*(buf + 2) = cost;
			}*/
			
			bool contains(node_id_t id) const {
				return ( (n1 == id) || (n2 == id) );
			}

			friend bool operator== (const edge &e1, const edge &e2) {
				return (e1.n1 == e2.n1 && e1.n2 == e2.n2) || (e1.n2 == e2.n1 && e1.n1 == e2.n2);
			}

			friend bool operator!= (const edge &e1, const edge &e2) {return !(e1 == e2); }
			friend bool operator< (const edge &e1, const edge &e2) { return &e1 < &e2; }

		};

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

		int add_edge(const edge &e);
		
		bool network_ready() const;
	
		void bcast_reset();

		void print_table(node_id_t node_id);

	private:

		void on_node_msg(int msglen, char *msg, const struct sockaddr_in *sin);
		
		void on_req_init(node_id_t node_id, const struct sockaddr_in *sin);
		void on_tbl_info(node_id_t, const char *buf, int buflen);

		void print_tbl_upd_msg() const;

		void send_table(node_id_t node_id) const;
		void send_cost_change(const edge *e, node_id_t node_id) const;

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

		std::map<node_id_t, struct sockaddr_in> m_nodes;
		std::set<edge> m_edges;

		pthread_mutex_t m_tbl_upd_mtx;
		pthread_cond_t m_tbl_upd_cond;
		bool m_tbl_upd;
		char *m_tbl_upd_msg;
		int m_tbl_upd_msg_len;


}; /* coord */

}; /* net04 */
#endif /* COORD_H */
