#include "proto_coord.h"

#include <cerrno>
#include <string>
#include <cassert>

extern "C" {
#include <arpa/inet.h>
}

using namespace net04;

void proto_coord::request_coord_init(header_t *hdr, node_id_t id) {
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

void proto_coord::request_table(header_t *hdr) {
	hdr->type = TYPE_REQ_TBL;
	hdr->msg_len = 0;
	hdr->id = 0; /* coord gets node_id of zero */
}

void proto_coord::table_info(header_t *hdr, node_id_t node_id, int num_entries) {
	hdr->type = TYPE_TBL_INFO;
	hdr->msg_len = sizeof(table_info_t)*num_entries;
	hdr->id = node_id;
}

void proto_coord::net_reset(header_t *hdr) {
	hdr->type = TYPE_NET_RST;
	hdr->msg_len = 0;
	hdr->id = 0; /* coord gets node_id of zero */
}

void proto_coord::link_update(header_t *hdr) {
	hdr->type = TYPE_LNK_UPD;
	hdr->msg_len = sizeof(link_desc_t);
	hdr->id = 0; /* coord gets node_id of zero */
}

void proto_coord::hton_hdr(header_t *hdr) {
	assert(sizeof(node_id_t) == 1 );

	hdr->type = htons(hdr->type);
	hdr->msg_len = htons(hdr->msg_len);
}

void proto_coord::ntoh_hdr(header_t *hdr) {
	assert(sizeof(node_id_t) == 1 );

	hdr->type = ntohs(hdr->type);
	hdr->msg_len = ntohs(hdr->msg_len);
}


uint16_t proto_coord::msg_type(const char *msg) {
	const header_t *hdr = (header_t *) msg;

	return hdr->type;
}

uint16_t proto_coord::msg_len(const char *msg) {
	const header_t *hdr = (header_t *) msg;

	return hdr->msg_len;
}

node_id_t proto_coord::msg_node_id(const char *msg) {
	const header_t *hdr = (header_t *) msg;

	return hdr->id;
}

const char *proto_coord::type_to_str(uint16_t type) {
	const char *ret = "unknown";
	if( (type >= 0) && (type < num_of_types) ) {
		ret = type_str[type];
	}
		
	return ret;
}
