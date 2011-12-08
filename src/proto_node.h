#ifndef PROTO_NODE_H
#define PROTO_NODE_H

#include "net04_types.h"
#include "proto_base.h"

namespace net04 {

namespace proto_node {

	static const int max_msg_len = 1024;
	static const int max_route_size = 10;	
	
	static const uint16_t TYPE_FWD_MSG = 1;
	static const uint16_t TYPE_ROUTE_INFO = 2;

	struct msg_header_t {
		uint32_t msg_id;
		node_id_t src;
		node_id_t dest;
		node_id_t route[max_route_size];
	};

	void route_info(proto_base::header_t *hdr, node_id_t id);

	const char *type_to_str(uint16_t type);
	void print_msg(const char *buf);

	uint32_t mhdr_msg_id(const char *buf);
	node_id_t mhdr_src(const char *buf);
	node_id_t mhdr_dest(const char *buf);
	int mhdr_add_to_route_list(char *buf, node_id_t node_id);
	void mhdr_print_route_list(char *buf);
	void mhdr_zero_route_list(char *buf);
	
	void hton_msg_hdr(msg_header_t *hdr);
	void ntoh_msg_hdr(msg_header_t *hdr);

}; /* proto_node */


}; /* net04 */

#endif /* PROTO_NODE_H */