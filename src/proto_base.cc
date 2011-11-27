#include "proto_base.h"

#include "net04_common.h"

using namespace net04;

void proto_base::send_udp_msg(int socket, const struct sockaddr_in *addr, int msglen, const char *msg) {
	int bytes;

	if( (bytes = sendto(socket, msg, msglen, 0, (struct sockaddr *)addr, sizeof(struct sockaddr_in)) ) != msglen ) {
		if(bytes == 0) {
			FATAL("socket is closed");
		}

		FATAL("could not send entire message");
	}	
}
