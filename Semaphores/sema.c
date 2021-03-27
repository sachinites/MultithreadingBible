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

sema_t *
sema_get_new_semaphore() {

    sema_t *sema = calloc(1, sizeof(sema_t));
    return sema;
}

void
sema_init(sema_t *sema, int permit_counter) {

    sema->permit_counter = permit_counter;
    pthread_cond_init(&sema->cv, NULL);
    pthread_mutex_init(&sema->mutex, NULL);
}


/*
 * Without using Pending Signals
 * */
void
sema_wait(sema_t *sema) {

   pthread_mutex_lock(&sema->mutex);
   sema->permit_counter--;
   if (sema->permit_counter < 0) {
       pthread_cond_wait(&sema->cv, &sema->mutex);
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
    }
    pthread_mutex_unlock(&sema->mutex);
}

void
sema_destroy(sema_t *sema) {

    sema->permit_counter = 0;
    pthread_mutex_unlock(&sema->mutex);
    pthread_cond_destroy(&sema->cv);	
    pthread_mutex_destroy(&sema->mutex);
}

int
sema_getvalue(sema_t *sema) {

	return sema->permit_counter;
}
