#ifndef __RW_LOCKS__
#define __RW_LOCKS__

#include <pthread.h>
#include <stdint.h>

typedef enum lock_type_ {

	read_lock_t,
	write_lock_t,
	unlock_t
} lock_type_t;

typedef struct rw_lock_ {

	lock_type_t lock_status;
	pthread_mutex_t mutex_lock_status;
	pthread_cond_t cv;
	uint16_t n;  /* no of threads in C.S. */
} rw_lock_t;


void
rw_lock_init (rw_lock_t *rw_lock);

void
rw_lock_rd_lock (rw_lock_t *rw_lock);

void
rw_lock_rd_unlock (rw_lock_t *rw_lock) ;

void
rw_lock_wr_lock (rw_lock_t *rw_lock);

void
rw_lock_wr_unlock (rw_lock_t *rw_lock) ;

#endif 