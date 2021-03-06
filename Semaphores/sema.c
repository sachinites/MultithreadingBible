/*
 * =====================================================================================
 *
 *       Filename:  sema.c
 *
 *    Description: This file implements the Semaphores 
 *
 *        Version:  1.0
 *        Created:  01/31/2021 10:01:17 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ABHISHEK SAGAR (), sachinites@gmail.com
 *   Organization:  Juniper Networks
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <stdbool.h>
#include "sema.h"

struct sema_ {

    int permit_counter;
	int pending_signals;
    pthread_cond_t cv;
    pthread_mutex_t mutex;
	pthread_mutex_t destroy_sema_mutex;
};

sema_t *
sema_get_new_semaphore() {

	sema_t *sema = calloc(1, sizeof(sema_t));
	return sema;
}

void
sema_init(sema_t *sema, int permit_counter) {

	sema->permit_counter = permit_counter;
	sema->pending_signals = 0;
	pthread_cond_init(&sema->cv, NULL);
	pthread_mutex_init(&sema->mutex, NULL);
	pthread_mutex_init(&sema->destroy_sema_mutex, NULL);
}

/* Do while loop implementation */
void
sema_wait(sema_t *sema) {
	/* 
 	 * lock the mutex so that state of semaphore cannot be
 	 * changed by any other thread
 	 * */
	pthread_mutex_lock(&sema->mutex);
	
	/* Calling thread is trying to enter into C.S, decrease the
 	 * semaphore */
	sema->permit_counter--;

	/* If after decrement the Semaphore permit_counter is -ve, then calling thread
 	 *  must block*/
	if (sema->permit_counter < 0) {

		do {
			/* Semaphore permit_counter is  -ve, not permitted to enter into C.S*/
			pthread_cond_wait(&sema->cv, &sema->mutex);

			/* Calling thread has woken up, enter into C.S only when
 			 * pending signal is available, else continue to stay blocked*/
		} while(sema->pending_signals < 1);

		/* Got the license to enter into C.S, consume one pending signal */
		sema->pending_signals--;
	}
	pthread_mutex_unlock(&sema->mutex);
}

/* While loop implementation */
void
sema_wait2(sema_t *sema) {
	/* 
 	 * lock the mutex so that state of semaphore cannot be
 	 * changed by any other thread
 	 * */
	pthread_mutex_lock(&sema->mutex);
	
	/* Calling thread is trying to enter into C.S, decrease the
 	 * semaphore */
	sema->permit_counter--;

	/* If after decrement the Semaphore permit_counter is -ve, then calling thread
 	 *  must block*/
	if (sema->permit_counter < 0) {

		while(sema->pending_signals < 1) {
			
			/* Semaphore permit_counter is  -ve, not permitted to enter into C.S*/
			pthread_cond_wait(&sema->cv, &sema->mutex);

			/* Calling thread has woken up, enter into C.S only when
 			 * pending signal is available, else continue to stay blocked*/
		}

		/* Got the license to enter into C.S, consume one pending signal */
		sema->pending_signals--;
	}
	pthread_mutex_unlock(&sema->mutex);
}

void
sema_post(sema_t *sema) {

	bool any_thread_waiting;

	pthread_mutex_lock(&sema->mutex);

	any_thread_waiting = sema->permit_counter < 0 ? true : false;

	sema->permit_counter++;

	if (any_thread_waiting) {
		pthread_cond_signal(&sema->cv);
		sema->pending_signals++;
	}

	pthread_mutex_unlock(&sema->mutex);
}

#if 0
void
sema_wait(sema_t *sema) {

	pthread_mutex_lock(&sema->mutex);
	sema->permit_counter--;
	while (sema->permit_counter < 0) {
		pthread_cond_wait(&sema->cv, &sema->mutex);
		if (sema->pending_signals > 0){
			sema->pending_signals--;
			break;
		}
	}
	pthread_mutex_unlock(&sema->mutex);
}
#endif

void
sema_destroy(sema_t *sema) {

	pthread_mutex_lock(&sema->destroy_sema_mutex);
	sema->permit_counter = 0;
	sema->pending_signals = 0;
	pthread_cond_destroy(&sema->cv);	
	pthread_mutex_destroy(&sema->mutex);
	pthread_mutex_unlock(&sema->destroy_sema_mutex);
	pthread_mutex_destroy(&sema->destroy_sema_mutex);
}

int
sema_getvalue(sema_t *sema) {

	return sema->permit_counter;
}
