#ifndef COORD_PROTO_H
#define COORD_PROTO_H

#include <cerrno>
#include <string>

extern "C" {
#include <unistd.h>
}

#include "node.h"

namespace net04 {

namespace coord_proto {
	
	const int max_msg_len = 256;
	struct header {
		node_id_t 
	};

}; /* coord_proto */

}; /* net04 */
#endif /* COORD_PROTO_H */