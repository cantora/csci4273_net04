#include "net04_common.h"

extern "C" {
#include <fcntl.h>
}

void net04::set_nonblocking(int fd) {
	int opts;

	opts = fcntl(fd,F_GETFL);
	if (opts < 0) {
		FATAL(NULL);
	}

	opts = (opts | O_NONBLOCK);
	if (fcntl(fd,F_SETFL,opts) < 0) {
		FATAL(NULL);
	}

	return;
}

void net04::print_hex_bytes(const char *buf, int len) {
	int i;

	for(i = 0; i < len; i++) {
		printf("0x%02x ", buf[i]);
	}
}
