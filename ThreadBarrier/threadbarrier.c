
#include <stdio.h>
#include <unistd.h>
#include "threadbarrier.h"

void
thread_barrier_print(th_barrier_t *th_barrier) {
    
    printf("th_barrier->max_count = %u\n", th_barrier->max_count);
    printf("th_barrier->curr_wait_count = %u\n", th_barrier->curr_wait_count);
    printf("th_barrier->is_ready_again = %s\n", th_barrier->is_ready_again ? "true" : "false");
}

void
thread_barrier_init ( th_barrier_t *barrier, uint32_t count) {
    
    barrier->max_count = count;
    barrier->curr_wait_count = 0;
    pthread_cond_init(&barrier->cv, NULL);
    pthread_mutex_init(&barrier->mutex, NULL);
    barrier->is_ready_again = true;
    pthread_cond_init(&barrier->busy_cv, NULL);
}

void

thread_barrier_signal_all ( th_barrier_t *barrier) {

	pthread_mutex_lock (&barrier->mutex);

	if ( barrier->is_ready_again == false ||

        barrier->curr_wait_count == 0 ) {
		pthread_mutex_unlock (&barrier->mutex);
		return;
    }

	pthread_cond_signal(&barrier->cv);
	pthread_mutex_unlock (&barrier->mutex);	
}

void
thread_barrier_barricade ( th_barrier_t *barrier) {

	pthread_mutex_lock (&barrier->mutex);

	if (barrier->is_ready_again == false ) {
		pthread_cond_wait(&barrier->busy_cv, 
		                  &barrier->mutex);
	}

	if ( barrier->curr_wait_count + 1 == barrier->max_count ) {

		barrier->is_ready_again = false;
		pthread_cond_signal(&barrier->cv);
		pthread_mutex_unlock (&barrier->mutex);
		return;
	}

	barrier->curr_wait_count++;
	pthread_cond_wait(&barrier->cv, &barrier->mutex);
	barrier->curr_wait_count--;

	if (barrier->curr_wait_count == 0) {
		barrier->is_ready_again = true;
		pthread_cond_broadcast(&barrier->busy_cv);
    }
	else {
		pthread_cond_signal(&barrier->cv);
	}
	pthread_mutex_unlock (&barrier->mutex);
}




