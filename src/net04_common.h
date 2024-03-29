#ifndef NET04_COMMON_H
#define NET04_COMMON_H

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>

#define FATAL(s) printf("fatal error at %s(%d)", __FILE__, __LINE__); \
	if(s != NULL) printf(" (%s): ", s); else printf(": "); \
	printf("%s\n", strerror(errno)); \
	exit(1);

#ifdef NET04_DEBUG_LOG
#define NET04_LOG(fmt, ...) printf(fmt, ## __VA_ARGS__ )
#else
#define NET04_LOG(fmt, ...) ;
#endif

namespace net04 {
	void set_nonblocking(int fd);

	void print_hex_bytes(const char *buf, int len);

	int p_mutex_lock(pthread_mutex_t *mutex);
	int p_mutex_unlock(pthread_mutex_t *mutex);
	int p_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
	int p_cond_timedwait_usec(pthread_cond_t *cond, pthread_mutex_t *mutex, int usec_timeout);
	int p_cond_signal(pthread_cond_t *cond);

}

#endif /* NET04_COMMON_H */
