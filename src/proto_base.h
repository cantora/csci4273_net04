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

namespace net04 {

namespace proto_base {
	void send_udp_msg(int socket, const struct sockaddr_in *addr, int msglen, const char *msg);
	
}; /* proto_base */

}; /* net04 */
#endif /* PROTO_BASE_H */