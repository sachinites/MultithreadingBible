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

#include <pthread.h>
#include <assert.h>
#include "sema.h"

struct sema_ {

    int count;
	int max_count;
    pthread_cond_t cv;
    pthread_mutex_t mutex;
	pthread_mutex_t destroy_sema_mutex;
};

void
sema_init(sema_t *sema, int count) {

	sema->count = count;
	sema->max_count = count;
	pthread_cond_init(&sema->cv, NULL);
	pthread_mutex_init(&sema->mutex, NULL);
	pthread_mutex_init(&sema->destroy_sema_mutex, NULL);
}

void
sema_wait(sema_t *sema) {

	pthread_mutex_lock(&sema->mutex);
	sema->count--;
	if (sema->count < 0) {
		pthread_cond_wait(&sema->cv, &sema->mutex);
	}
	pthread_mutex_unlock(&sema->mutex);
}

void
sema_post(sema_t *sema) {

	pthread_mutex_lock(&sema->mutex);
	if (sema->count == sema->max_count) {
		assert(0);
	}
	sema->count++;
	pthread_cond_signal(&sema->cv);
	pthread_mutex_unlock(&sema->mutex);
}

void
sema_destroy(sema_t *sema) {

	pthread_mutex_lock(&sema->destroy_sema_mutex);
	sema->count = 0;
	sema->max_count = 0;
	pthread_cond_destroy(&sema->cv);	
	pthread_mutex_destroy(&sema->mutex);
	pthread_mutex_unlock(&sema->destroy_sema_mutex);
	pthread_mutex_destroy(&sema->destroy_sema_mutex);
}

int
sema_getvalue(sema_t *sema) {

	return sema->count;
}
