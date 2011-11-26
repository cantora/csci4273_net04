#ifndef NODE_H
#define NODE_H

#include <cerrno>
#include <string>
#include <map>

extern "C" {
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netdb.h> 
#include <arpa/inet.h>
}

#include "thread_pool.h"

namespace net04 {

class node {
	public:
		node(node_id_t node_id, const struct sockaddr_in *coord_addr, 
			struct sockaddr_in *coord_sin, struct sockaddr_in *dv_sin);
		~node();

		typedef node_id_t uint8_t;
		typedef cost_t uint8_t;

		static void listen_coord(void *instance);
		static void listen_dv(void *instance);
		
	private:

		const node_id_t m_node_id;

		const struct sockaddr_in *const m_coord_addr;

		int m_dv_socket;
		struct sockaddr_in *const m_dv_sin;

		int m_coord_socket;
		struct sockaddr_in *const m_coord_sin;

		std::map<node_id_t, cost_t> m_rtable;

		net02::thread_pool *const m_pool;

}; /* node */

}; /* net04 */
#endif /* NODE_H */