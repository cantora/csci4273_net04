#ifndef COORD_H
#define COORD_H

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

class coord {
	public:
		coord(struct sockaddr_in *sin);
		~coord();

		static void listen_node(void *instance);
		
		unsigned short port() const {
			return ntohs(m_sin->sin_port);
		}

		
	private:
		void on_node_msg(int msglen, const char *msg, const struct sockaddr_in *sin);

		int m_socket;
		struct sockaddr_in *const m_sin;

		net02::thread_pool *const m_pool;

		std::map<uint32_t, struct sockaddr_in> m_nodes;

}; /* coord */

}; /* net04 */
#endif /* COORD_H */