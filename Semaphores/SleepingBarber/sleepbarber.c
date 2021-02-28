/*
 * =====================================================================================
 *
 *       Filename:  sleepbarber.c
 *
 *    Description: This file implements sleeping barber problem 
 *
 *        Version:  1.0
 *        Created:  02/26/2021 09:53:11 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ABHISHEK SAGAR (), sachinites@gmail.com
 *   Organization:  Juniper Networks
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <assert.h>
#include "sleepbarber.h"

#define N_CHAIRS 5

/* Globals */

/* A mutex required to access chair arrays in a mutual exclusive way*/
pthread_mutex_t chair_global_mutex;
/* Array of chairs available in shop */
chair_t chair_array[N_CHAIRS];

/*
 * Get the waiting customer with the least token.
 * While we are searching for the right customer to serve next,
 * no other thread must change the state of any customer while
 * performing this activity.
 */
customer_t *
customer_get_next_waiting_customer() {

	int i;
	chair_t *chair;
	customer_t *customer;

	customer = NULL;

	pthread_mutex_lock(&chair_global_mutex);

	for ( i = 0; i < N_CHAIRS; i++) {

		chair = &chair_array[i];

		if (chair->chair_state == CHAIR_AVAILABLE) {
			continue;
		}

		assert(chair->customer);
		customer = chair->customer;			
		break;
	}		
	pthread_mutex_unlock(&chair_global_mutex);
	return customer;
}


void *
barber_fn(void *arg) {

	barber_t *barber = (barber_t *)arg;
	return NULL;
}

void
barber_init(barber_t *barber,
			void *(*barber_fn)(void *),
			void *arg) {

	pthread_mutex_init(&barber->state_mutex, NULL);
	barber->barber_state = BARBER_SLEEPING;
	barber->customer = NULL;
	pthread_cond_init(&barber->cv, NULL);
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);	
	pthread_create(&barber->thread_handle,
				   &attr, barber_fn, (void *)barber);
}

/* Barber must continue to sleep until he is
 * assigned as customer */
void
barber_sleep(barber_t *barber) {
	
	pthread_mutex_lock(&barber->state_mutex);

	while(barber->customer == NULL) {
		pthread_cond_wait(&barber->cv, &barber->state_mutex);
	}

	pthread_mutex_unlock(&barber->state_mutex);

	barber_assign_customer(barber, customer_get_next_waiting_customer());
}

static void
barber_service_customer(barber_t *barber) {

	printf("Barber Servicing Customer %u\n", barber->customer->token_no);
	sleep(2);
}

/* Assign a customer to the barber, this fn assumes, barber was
 * asleep and customer was waiting */
bool
barber_assign_customer(barber_t *barber, customer_t *customer) {

	pthread_mutex_lock(&barber->state_mutex);

	if(barber->barber_state != BARBER_SLEEPING) {
		assert(barber->customer);
		pthread_mutex_unlock(&barber->state_mutex);
		return false;
	}

	assert(barber->customer == NULL);

	pthread_mutex_lock(&customer->state_mutex);
	if(customer->customer_state != CUSTOMER_WAITING) {
		pthread_mutex_unlock(&customer->state_mutex);
		return false;
	}

	pthread_mutex_lock(&chair_global_mutex);
	pthread_mutex_lock(&chair->state_mutex);
	assert(customer->chair->chair_state == CHAIR_OCCUPIED);
	assert(customer->chair->customer == customer);

	barber->customer = customer;
	barber->barber_state = BARBER_BUSY;
	customer->customer_state = CUSTOMER_BUSY;
	
	
	barber_service_customer(barber);
	
	
}

/* Barber must be sleeping only when there is no customer to server.
 * Barber should release the customer,
 * and Customer should release the chair
 */
customer_t *
barber_release_customer(barber_t *barber) {

	chair_t *chair;
	customer_t *customer;

	pthread_mutex_lock(&barber->state_mutex);
	assert(barber->barber_state != BARBER_SLEEPING);
	assert(barber->customer);
	barber->barber_state = BARBER_SLEEPING;
	customer = barber->customer;
	barber->customer = NULL;

	pthread_mutex_lock(&customer->state_mutex);
	assert(customer->customer_state == CUSTOMER_BUSY);
	customer->customer_state = CUSTOMER_DONE;
	chair = customer->chair;

	pthread_mutex_lock(&chair_global_mutex);
	pthread_mutex_lock(&chair->state_mutex);
	assert(chair->chair_state == CHAIR_OCCUPIED);
	chair->chair_state = CHAIR_AVAILABLE;
	assert(chair->customer == customer);
	chair->customer = NULL;

	/* Mutex must be unlocked in the reverse order, LIFO order */
	pthread_mutex_unlock(&chair->state_mutex);
	pthread_mutex_unlock(&chair_global_mutex);
	pthread_mutex_unlock(&customer->state_mutex);
	pthread_mutex_unlock(&barber->state_mutex);
	
	/* Preserve the same locking order at all places in
 	 * the program, else deadlock may happen*/
	return customer;
}
