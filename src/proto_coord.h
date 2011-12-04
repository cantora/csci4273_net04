#ifndef PROTO_COORD_H
#define PROTO_COORD_H

#include "net04_types.h"
#include "proto_base.h"


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
	static const uint16_t TYPE_SND_MSG = 9;
	static const uint16_t TYPE_FWD_ERR = 10;
	static const uint16_t TYPE_FWD_ACK = 11;

	static const char *type_str[] = {"error", "request_init", "net_size", "ok", "reg_ack", "net_rst", "lnk_upd", "tbl_info", "req_tbl", "snd_msg", "fwd_err", "fwd_ack"};
	static const int num_of_types = 12;

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

	struct snd_msg_t {
		uint32_t msg_id;
		node_id_t dest;
	};

	const char *type_to_str(uint16_t type);

	void request_coord_init(proto_base::header_t *hdr, node_id_t id);
	void reply_net_size(proto_base::header_t *hdr);
	void reply_err(proto_base::header_t *hdr);
	void reply_ok(proto_base::header_t *hdr);
	void reply_reg_ack(proto_base::header_t *hdr);
	void request_table(proto_base::header_t *hdr);
	void table_info(proto_base::header_t *hdr, node_id_t node_id, int num_entries);
	void send_msg(proto_base::header_t *hdr, uint16_t msglen);

	void net_reset(proto_base::header_t *hdr);
	void link_update(proto_base::header_t *hdr);
	
}; /* proto_coord */


}; /* net04 */

#endif /* PROTO_COORD_H */