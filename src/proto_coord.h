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
	
	typedef node::node_id_t net_size_t;

	static const int max_msg_len = 1024;
	
	static const uint16_t TYPE_ERR = 0;
	static const uint16_t TYPE_REQ_INIT = 1;
	static const uint16_t TYPE_NET_SIZE = (1 << 1);
	static const uint16_t TYPE_OK = (1 << 2);
	static const char *type_str[] = {"error", "request_init", "net_size", "ok"};

	struct header_t {
		uint16_t type;
		uint16_t msg_len;		
		node::node_id_t id;
	};

	void request_coord_init(header_t *hdr, node::node_id_t id);
	void reply_net_size(header_t *hdr);
	void reply_err(header_t *hdr);
	void reply_ok(header_t *hdr);

	void hton_hdr(header_t *hdr);
	void ntoh_hdr(header_t *hdr);

	uint16_t msg_type(const char *msg);
	node::node_id_t msg_node_id(const char *msg);

}; /* proto_coord */

}; /* net04 */
#endif /* PROTO_COORD_H */