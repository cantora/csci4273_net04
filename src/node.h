#ifndef NODE_H
#define NODE_H

#include <cerrno>
#include <string>
#include <map>
#include <cassert>

extern "C" {
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netdb.h> 
#include <arpa/inet.h>
}

#include "thread_pool.h"
#include "net04_types.h"
#include "proto_coord.h"
#include "proto_base.h"

namespace net04 {

class node {
	public:
		
		static const cost_t INF_COST = 0x00;		
		
		node(node_id_t node_id, const struct sockaddr_in *coord_addr, 
			struct sockaddr_in *coord_sin, struct sockaddr_in *dv_sin);
		~node();

		static void listen_coord(void *instance);
		static void listen_dv(void *instance);
		
	private:

		void send_coord_msg(const char *buf, int buflen) const;
		void request_coord_init() const;
		void on_coord_msg(int msglen, char *msg);
		void on_request_table() const;

		void on_link_update(proto_coord::header_t *hdr, uint16_t msg_len, int buflen, const char *buf);
		
		const node_id_t m_node_id;

		const struct sockaddr_in *const m_coord_addr;

		int m_dv_socket;
		struct sockaddr_in *const m_dv_sin;

		int m_coord_socket;
		struct sockaddr_in *const m_coord_sin;

		std::map<node_id_t, fwd_entry_t > m_dv_table;
		std::map<node_id_t, std::pair<struct sockaddr_in, cost_t> > m_links;

		net02::thread_pool *const m_pool;

		bool m_registered;

}; /* node */

}; /* net04 */
#endif /* NODE_H */