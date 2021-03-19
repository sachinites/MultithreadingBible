#ifndef __TH_BARRIER__
#define __TH_BARRIER__

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

typedef struct th_barrier_ {

 	uint32_t max_count;
	uint32_t curr_wait_count;
	pthread_cond_t cv;
	pthread_mutex_t mutex;
	bool is_ready_again;
	pthread_cond_t busy_cv;
} th_barrier_t;

void
thread_barrier_print(th_barrier_t *th_barrier);

void
thread_barrier_init ( th_barrier_t *barrier, uint32_t count);

void
thread_barrier_signal_all ( th_barrier_t *barrier);
                     
void
thread_barrier_barricade ( th_barrier_t *barrier);
                     
#endif 