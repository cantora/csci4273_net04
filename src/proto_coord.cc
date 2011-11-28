#include "proto_coord.h"

using namespace net04;

void proto_coord::request_coord_init(header_t *hdr, node::node_id_t id) {
	hdr->type = TYPE_REQ_INIT;
	hdr->msg_len = 0;
	hdr->id = id;
}

void proto_coord::reply_net_size(header_t *hdr) {
	hdr->type = TYPE_NET_SIZE;
	hdr->msg_len = sizeof(net_size_t); 
	hdr->id = 0; /* coord gets node_id of zero */
}

void proto_coord::reply_err(header_t *hdr) {
	hdr->type = TYPE_ERR;
	hdr->msg_len = 0;
	hdr->id = 0; /* coord gets node_id of zero */
}

void proto_coord::reply_ok(header_t *hdr) {
	hdr->type = TYPE_OK;
	hdr->msg_len = 0;
	hdr->id = 0; /* coord gets node_id of zero */
}

void proto_coord::reply_reg_ack(header_t *hdr) {
	hdr->type = TYPE_REG_ACK;
	hdr->msg_len = 0;
	hdr->id = 0; /* coord gets node_id of zero */
}

void proto_coord::hton_hdr(header_t *hdr) {
	assert(sizeof(node::node_id_t) == 1 );

	hdr->type = htons(hdr->type);
	hdr->msg_len = htons(hdr->msg_len);
}

void proto_coord::ntoh_hdr(header_t *hdr) {
	assert(sizeof(node::node_id_t) == 1 );

	hdr->type = ntohs(hdr->type);
	hdr->msg_len = ntohs(hdr->msg_len);
}


uint16_t proto_coord::msg_type(const char *msg) {
	const header_t *hdr = (header_t *) msg;

	return hdr->type;
}

node::node_id_t proto_coord::msg_node_id(const char *msg) {
	const header_t *hdr = (header_t *) msg;

	return hdr->id;
}