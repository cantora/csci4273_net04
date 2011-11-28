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

namespace net04 {

class coord {
	public:

		class edge {
			public:
			node::node_id_t n1;
			node::node_id_t n2;
			node::cost_t cost;

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
	private:

		void on_node_msg(int msglen, char *msg, const struct sockaddr_in *sin);

		void reply_net_size(node::node_id_t node_id);
		void reply_ok(node::node_id_t node_id);
		void reply_err(node::node_id_t node_id, const struct sockaddr_in *sin);
		void reply_reg_ack(node::node_id_t node_id);
		void reply_msg(node::node_id_t node_id, const char *buf, int buflen, const struct sockaddr_in *sin = NULL);

		int m_socket;
		struct sockaddr_in *const m_sin;

		net02::thread_pool *const m_pool;

		std::map<node::node_id_t, struct sockaddr_in> m_nodes;
		std::set<edge> m_edges;

}; /* coord */

}; /* net04 */
#endif /* COORD_H */
