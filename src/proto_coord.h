#ifndef PROTO_COORD_H
#define PROTO_COORD_H

#include "net04_types.h"

namespace net04 {

namespace proto_coord {
	
	typedef net04::node_id_t net_size_t;

	static const int max_msg_len = 1024;
	
	static const uint16_t TYPE_ERR = 0;
	static const uint16_t TYPE_REQ_INIT = 1;
	static const uint16_t TYPE_NET_SIZE = 2;
	static const uint16_t TYPE_OK = 3;
	static const uint16_t TYPE_REG_ACK = 4;
	static const uint16_t TYPE_NET_RST = 5;
	static const uint16_t TYPE_LNK_UPD = 6;
	static const uint16_t TYPE_TBL_INFO = 7;
	static const uint16_t TYPE_REQ_TBL = 8;

	static const char *type_str[] = {"error", "request_init", "net_size", "ok", "reg_ack", "net_rst", "lnk_upd", "tbl_info", "req_tbl"};
	static const int num_of_types = 9;

	struct header_t {
		uint16_t type;
		uint16_t msg_len;		
		node_id_t id;
	};

	struct link_desc_t {
		uint32_t s_addr;
		node_id_t id;
		cost_t cost;		
		uint16_t port;
	};

	struct table_info_t {
		fwd_entry_t link;
		node_id_t dest;
	};

	const char *type_to_str(uint16_t type);

	void request_coord_init(header_t *hdr, node_id_t id);
	void reply_net_size(header_t *hdr);
	void reply_err(header_t *hdr);
	void reply_ok(header_t *hdr);
	void reply_reg_ack(header_t *hdr);
	void request_table(header_t *hdr);
	void table_info(header_t *hdr, node_id_t node_id, int num_entries);

	void net_reset(header_t *hdr);
	void link_update(header_t *hdr);

	void hton_hdr(header_t *hdr);
	void ntoh_hdr(header_t *hdr);

	uint16_t msg_type(const char *msg);
	uint16_t msg_len(const char *msg);
	node_id_t msg_node_id(const char *msg);

}; /* proto_coord */


}; /* net04 */

#endif /* PROTO_COORD_H */