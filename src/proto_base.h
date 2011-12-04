#ifndef PROTO_BASE_H
#define PROTO_BASE_H

#include <cerrno>
#include <string>

extern "C" {
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netdb.h> 
#include <arpa/inet.h>
}

#include "net04_types.h"

namespace net04 {

namespace proto_base {

	struct header_t {
		uint16_t type;
		uint16_t msg_len;		
		node_id_t id;
	};
	
	void send_udp_msg(int socket, const struct sockaddr_in *addr, int msglen, const char *msg);

	void hton_hdr(header_t *hdr);
	void ntoh_hdr(header_t *hdr);

	uint16_t msg_type(const char *msg);
	uint16_t msg_len(const char *msg);
	node_id_t msg_node_id(const char *msg);
	
}; /* proto_base */

}; /* net04 */
#endif /* PROTO_BASE_H */