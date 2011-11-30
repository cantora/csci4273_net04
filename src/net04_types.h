#ifndef NET04_TYPES_H
#define NET04_TYPES_H

extern "C" {
#include <stdint.h>
}

namespace net04 {

  	typedef uint8_t node_id_t;
	typedef uint8_t cost_t;

	struct fwd_entry_t {
		node_id_t id;
		cost_t cost;
	};

}; /* net04 */

#endif /* NET04_TYPES_H */
