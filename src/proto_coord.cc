#include "proto_coord.h"

using namespace net04;

void proto_coord::request_coord_init(header_t &hdr, node::node_id_t id) {
	hdr.type = TYPE_REQ_INIT;
	hdr.msg_len = 0;
	hdr.id = id;
}

uint16_t proto_coord::msg_type(const char *msg) {
	const header_t *hdr = (header_t *) msg;

	return hdr->type;
}