/*
 * =====================================================================================
 *
 *       Filename:  sleepbarber.h
 *
 *    Description: This file declares the interface for Sleeping Barber Problem 
 *
 *        Version:  1.0
 *        Created:  02/26/2021 08:43:43 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ABHISHEK SAGAR (), sachinites@gmail.com
 *   Organization:  Juniper Networks
 *
 * =====================================================================================
 */

#ifndef __SLEEPING_BARBER__
#define __SLEEPING_BARBER__

/*
 * BARBER APIs
 */
typedef enum barber_state_{

	BARBER_SLEEPING,
	BARBER_BUSY
}barber_state_t;

typedef struct barber_ barber_t;
typedef struct customer_ customer_t;
typedef struct chair_ chair_t;

struct barber_{

	/* Barber is a thread, lets model all
 	 * living beings in this problem as threads*/
	pthread_t thread_handle;

	/* 
 	 * A Binary Semaphore to change the state of a
 	 * barber mutually exclusive way
 	 * */
	pthread_mutex_t state_mutex;
	barber_state_t barber_state;
	struct customer_ *customer;
	

	/* Barber has to sleep (blocked) when there is
	 * no customers, We need a CV for this*/
	pthread_cond_t cv;
};

void
barber_init(barber_t *barber, void *(*barber_fn)(void *), void *arg);

void
barber_sleep(barber_t *barber);

void
barber_wakeup(barber_t *barber);

bool
barber_assign_customer(barber_t *barber, customer_t *customer);

customer_t *
barber_release_customer(barber_t *barber);

barber_state_t
barber_get_state(barber_t *barber);

/* CUSTOMER APIS */

typedef enum customer_state_{

	CUSTOMER_ENTERING,
	CUSTOMER_WAITING,
	CUSTOMER_BUSY,
	CUSTOMER_DONE
} customer_state_t;

struct customer_{

	uint32_t token_no;
	/* Customer is a thread, lets model all
 	 * living beings in this problem as threads*/
	pthread_t thread_handle;

	/* 
 	 * A Binary Semaphore to change the state of a
 	 * customer in a mutually exclusive way
 	 * */
	pthread_mutex_t state_mutex;
	customer_state_t customer_state;
	chair_t *chair;	/* Customer is wairing on which charr */
	
	/* Customer has to wait (blocked) when barber is busy and
	 * empty chairs are there, We need a CV for this*/
	pthread_cond_t cv;
};

void
customer_init(customer_t *customer, void *(*customer_fn)(void *), void *arg);

void
customer_sleep(customer_t *customer);

void
customer_wakeup(customer_t *customer);

void
customer_assign_barber(customer_t *customer, barber_t *barber);

customer_state_t
customer_get_state(customer_t *customer);

customer_t *
customer_get_next_waiting_customer();

/* CHAIR APIs (Resource) */
typedef enum chair_state_ {

	CHAIR_AVAILABLE,
	CHAIR_OCCUPIED
} chair_state_t;

struct chair_ {

	int id;
	pthread_mutex_t state_mutex;	
	chair_state_t chair_state;
	customer_t *customer; /* Customer who is waiting on this char */
} ;

void
chair_init(chair_t *chair);

void
chair_occupy(chair_t *chair, customer_t *customer);

customer_t *
chair_release(chair_t *chair);

chair_state_t
chair_get_state(chair_t *chair);

#endif /* __SLEEPING_BARBER__  */
