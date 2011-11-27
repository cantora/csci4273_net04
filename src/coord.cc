#include "coord.h"

#include "net04_common.h"
#include "sock.h"
#include "proto_coord.h"

using namespace net04;
using namespace net02;
using namespace net01;
using namespace std;

coord::coord(struct sockaddr_in *sin) : m_sin(sin), m_pool(new thread_pool(2 + 8) ) {
	m_socket = sock::bound_udp_socket(m_sin);

	while(m_pool->dispatch_thread(listen_node, this, NULL) != 0) {
		usleep(1000);
	}

}

coord::~coord() {
	delete m_pool;

	close(m_socket);	
}

void coord::listen_node(void *instance) {
	coord *c = (coord *) instance;
	char msgbuf[1024];
	int msglen;
	struct sockaddr_in sin;
	socklen_t sinlen;

	NET04_LOG("listen_node: start\n");

	set_nonblocking(c->m_socket);

	while(1) {
		msglen = recvfrom(c->m_socket, msgbuf, sizeof(msgbuf), 0, (sockaddr *)&sin, &sinlen);
		if( msglen < 0) {
			if(errno == EAGAIN) {
				usleep(100000);
				continue;
			}

			FATAL(NULL);
		}
		else if(msglen == 0) {
			FATAL("socket was closed");
		}

		assert(sinlen > 0);

		c->on_node_msg(msglen, msgbuf, &sin);
	}

	assert(false); // shouldnt get here
}

void coord::on_node_msg(int msglen, const char *msg, const struct sockaddr_in *sin) {
	uint16_t type;

	assert(sin != NULL);

	type = proto_coord::msg_type(msg);

	NET04_LOG("received %d byte '%s' message from %s:%d\n", msglen, proto_coord::type_str[type], inet_ntoa(sin->sin_addr), ntohs(sin->sin_port) );

	
}
