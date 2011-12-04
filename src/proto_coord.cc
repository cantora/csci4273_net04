#include "proto_coord.h"
#include "proto_base.h"
#include "proto_node.h"

#include <cerrno>
#include <string>
#include <cassert>

extern "C" {
#include <arpa/inet.h>
}

#include "net04_common.h"

using namespace net04;
using namespace proto_base;

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

void proto_coord::send_msg(header_t *hdr, uint16_t msglen) {
	hdr->type = TYPE_SND_MSG;
	
	if(msglen > proto_node::max_msg_len - sizeof(node_id_t) ) { //( 1 << sizeof(uint16_t)*8 ) - 1
		FATAL("msg is too big");
	}
	hdr->msg_len = sizeof(proto_node::msg_header_t) + msglen;
	hdr->id = 0;
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

const char *proto_coord::type_to_str(uint16_t type) {
	const char *ret = "unknown";
	if( (type >= 0) && (type < num_of_types) ) {
		ret = type_str[type];
	}
		
	return ret;
}
