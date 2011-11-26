#include "coord.h"

#include "net04_common.h"
#include "sock.h"

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
	coord *n = (coord *) instance;

	NET04_LOG("listen_node: start\n");

		
	NET04_LOG("listen_node: stop\n");
}

