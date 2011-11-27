#ifndef PROTO_COORD_H
#define PROTO_COORD_H

#include <cerrno>
#include <string>

extern "C" {
#include <unistd.h>
}

#include "node.h"

namespace net04 {

namespace proto_coord {
	
	static const int max_msg_len = 1024;
	
	static const uint16_t TYPE_REQ_INIT = 1;

	static const char *type_str[] = {"none", "request_init"};

	struct header_t {
		uint16_t type;
		uint16_t msg_len;		
		node::node_id_t id;
	};

	void request_coord_init(header_t &hdr, node::node_id_t id);

	uint16_t msg_type(const char *msg);

}; /* proto_coord */

}; /* net04 */
#endif /* PROTO_COORD_H */