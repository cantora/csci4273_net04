#include "proto_base.h"

#include "net04_common.h"
#include <cassert>

using namespace net04;

void proto_base::send_udp_msg(int socket, const struct sockaddr_in *addr, int msglen, const char *msg) {
	int bytes;

	if( (bytes = sendto(socket, msg, msglen, 0, (struct sockaddr *)addr, sizeof(struct sockaddr_in)) ) != msglen ) {
		if(bytes == 0) {
			FATAL("socket is closed");
		}

		FATAL("could not send entire message");
	}	
}



void proto_base::hton_hdr(header_t *hdr) {
	assert(sizeof(node_id_t) == 1 );

	hdr->type = htons(hdr->type);
	hdr->msg_len = htons(hdr->msg_len);
}

void proto_base::ntoh_hdr(header_t *hdr) {
	assert(sizeof(node_id_t) == 1 );

	hdr->type = ntohs(hdr->type);
	hdr->msg_len = ntohs(hdr->msg_len);
}


uint16_t proto_base::msg_type(const char *msg) {
	const header_t *hdr = (header_t *) msg;

	return hdr->type;
}

uint16_t proto_base::msg_len(const char *msg) {
	const header_t *hdr = (header_t *) msg;

	return hdr->msg_len;
}

node_id_t proto_base::msg_node_id(const char *msg) {
	const header_t *hdr = (header_t *) msg;

	return hdr->id;
}
