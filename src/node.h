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
		void forward(char *msg, int msglen) const;

		node_id_t next_hop(node_id_t dest) const;
		
		void send_link_msg(node_id_t link, const char *buf, int buflen) const;
		void send_coord_fwd_ack(uint32_t msg_id, uint16_t type) const;
		void send_coord_msg(const char *buf, int buflen) const;
		void send_msg(int socket, const struct sockaddr_in *sin, const char *buf, int buflen) const;
		void request_coord_init() const;

		void route_bcast() const;
		void send_routes(node_id_t node_id) const;
		void send_route(node_id_t node_id, const fwd_entry_t *route) const;

		void on_node_msg(int msglen, char *msg, const struct sockaddr_in *sin);
		void on_fwd_message(char *msg, int msglen) const;
		void on_route_info(char *msg, int msglen);


		void on_coord_msg(int msglen, char *msg);
		void on_request_table() const;

		void on_link_update(proto_base::header_t *hdr, uint16_t msg_len, int buflen, const char *buf);
		void on_send_message(char *msg, int msglen) const;
		void on_message_receive(const char *msg, int msglen) const;

		const node_id_t m_node_id;

		const struct sockaddr_in *const m_coord_addr;

		int m_dv_socket;
		struct sockaddr_in *const m_dv_sin;

		int m_coord_socket;
		struct sockaddr_in *const m_coord_sin;

		/* key is destination node and value is a pair of link+cost of the aggregate links to node */
		typedef std::map<node_id_t, fwd_entry_t > dv_map_t;
		dv_map_t m_dv_table;
	
		typedef std::map<node_id_t, std::pair<struct sockaddr_in, cost_t> > link_map_t;
		link_map_t m_links;

		net02::thread_pool *const m_pool;

		bool m_registered;

}; /* node */

}; /* net04 */
#endif /* NODE_H */