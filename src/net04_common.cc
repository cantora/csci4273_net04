#include "net04_common.h"

extern "C" {
#include <fcntl.h>
#include <pthread.h>
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

int net04::p_mutex_lock(pthread_mutex_t *mutex) {
	int status;

	status = pthread_mutex_lock(mutex);

	if(status != 0) {
		FATAL("lock error");
	}

	return status;
}

int net04::p_mutex_unlock(pthread_mutex_t *mutex) {
	int status;

	status = pthread_mutex_unlock(mutex);

	if(status != 0) {
		FATAL("unlock error");
	}

	return status;
}

int net04::p_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) {
	int status;

	status = pthread_cond_wait(cond, mutex);

	if(status != 0) {
		FATAL("cond_wait error");
	}

	return status;
}

int net04::p_cond_signal(pthread_cond_t *cond) {
	int status;
	
	status = pthread_cond_signal(cond);
	if(status != 0) {
		FATAL("cond_signal error");
	}

	return status;
}