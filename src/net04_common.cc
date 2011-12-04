#include "net04_common.h"

extern "C" {
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
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

int net04::p_cond_timedwait_usec(pthread_cond_t *cond, pthread_mutex_t *mutex, int usec_timeout) {
	int status;
	struct timespec ts;
	struct timeval tp;
	int secs = usec_timeout/1000000;
	int usecs = usec_timeout % 1000000;
	int new_usecs;

	if(gettimeofday(&tp, NULL) != 0) {
		FATAL("gettimeofday");
	}

	ts.tv_sec  = tp.tv_sec;
	ts.tv_nsec = 0;

	if(usecs + tp.tv_usec >= 1000000) {
		ts.tv_sec += 1;
	}
	
	ts.tv_sec += secs;
	ts.tv_nsec += ((usecs + tp.tv_usec) % 1000000) * 1000;
	
	status = pthread_cond_timedwait(cond, mutex, &ts);

	if(status != 0 && status != ETIMEDOUT) {
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