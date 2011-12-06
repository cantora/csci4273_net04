#include "proto_node.h"

#include <cassert>
#include <cstring>
#include <cstdio>

using namespace net04;

void proto_node::route_info(proto_base::header_t *hdr, node_id_t id) {
	hdr->type = TYPE_ROUTE_INFO;
	hdr->msg_len = sizeof(fwd_entry_t);
	hdr->id = id;
}

void proto_node::print_msg(const char *buf) {
	uint16_t msglen = proto_base::msg_len(buf) - sizeof(msg_header_t);
	const char *msg = buf + sizeof(proto_base::header_t) + sizeof(msg_header_t);
	//msg_header_t *mhdr = (msg_header_t *) (buf + sizeof(proto_base::header_t) );
	int i;

	for(i = 0; i < msglen; i++) {
		fputc(msg[i], stdout);
	}
}

uint32_t proto_node::mhdr_msg_id(const char *buf) {
	msg_header_t *mhdr = (msg_header_t *) (buf + sizeof(proto_base::header_t) );

	return mhdr->msg_id;
}

node_id_t proto_node::mhdr_src(const char *buf) {
	msg_header_t *mhdr = (msg_header_t *) (buf + sizeof(proto_base::header_t) );

	return mhdr->src;
}

node_id_t proto_node::mhdr_dest(const char *buf) {
	msg_header_t *mhdr = (msg_header_t *) (buf + sizeof(proto_base::header_t) );

	return mhdr->dest;
}

int proto_node::mhdr_add_to_route_list(char *buf, node_id_t node_id) {
	msg_header_t *mhdr = (msg_header_t *) (buf + sizeof(proto_base::header_t) );
	int i;

	for(i = 0; (i < max_route_size) && (mhdr->route[i] != 0); i++) {}

	if(i >= max_route_size) {
		return -1;
	}

	mhdr->route[i] = node_id;

	return i;
}

void proto_node::mhdr_zero_route_list(char *buf) {
	msg_header_t *mhdr = (msg_header_t *) (buf + sizeof(proto_base::header_t) );
	memset(mhdr->route, 0, max_route_size);
}

void proto_node::hton_msg_hdr(msg_header_t *hdr) {
	assert(sizeof(node_id_t) == 1 );

	hdr->msg_id = htonl(hdr->msg_id);
}

void proto_node::ntoh_msg_hdr(msg_header_t *hdr) {
	assert(sizeof(node_id_t) == 1 );

	hdr->msg_id = ntohl(hdr->msg_id);
}

const char *proto_node::type_to_str(uint16_t type) {
	
	switch(type) {
		case TYPE_FWD_MSG :
			return "fwd_msg";
			break;
		case TYPE_ROUTE_INFO : 
			return "route_info";
			break;
		default:
			return "unknown type";
	
	}
}
