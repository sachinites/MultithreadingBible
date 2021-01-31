/*
 * =====================================================================================
 *
 *       Filename:  Assignment.c
 *
 *    Description: Assignment on Producer And Consumer 
 *
 *        Version:  1.0
 *        Created:  01/29/2021 10:27:27 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ABHISHEK SAGAR (), sachinites@gmail.com
 *   Organization:  Juniper Networks
 *
 * =====================================================================================
 */

#if 0

Problem Statement :
====================
Write a program which launches 4 threads - 2 consumer threads and 2 producer threads. Threads
are created in Detached Mode.
All 4 threads act on a shared resource - A Queue of integers. Producer threads produce
a random integer and add it to Queue, Consumer threads remove an integer from the Queue.
Maximum size of the Queue is 5.

Following are the constraints applied :

1. When producer threads produce an element and add it to the Queue, it does not release the Queue
untill the Queue is full i.e producer thread release the Queue only when it is full

2. When consumer threads consume an element from the Queue, it consumes the entire Queue and
do not release it until the Queue is empty.

3. Consumer Signals the Producers when Queue is Exhausted, Producers Signals the Consumers when Queue	
becomes full

4. Thread produce and consume elements into/from queue at the rate of 1s per element

Guidelines :
Use as many printfs as possible, so you can debug the program easily

Conmpile and Run :
gcc -g -c Queue.c -o Queue.o
gcc -g -c Assignment_prod_cons_on_Q.c -o Assignment_prod_cons_on_Q.o
gcc -g Assignment_prod_cons_on_Q.o Queue.o -o exe -lpthread
 
#endif

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#include "Queue.h"

static int new_int() {

	static int a = 0;
	a++;
	return a;
}

struct Queue_t *Q;

static const char *prod1 = "TP1";
static const char *prod2 = "TP2";
static const char *cons1 = "TC1";
static const char *cons2 = "TC2";

static void *
prod_fn(void *arg) {

	char *th_name = (char *)arg;

	printf("Thread %s waiting to lock the Queue\n", th_name);
	pthread_mutex_lock(&Q->q_mutex); /* S6 */
	printf("Thread %s locks the Queue\n", th_name);

	/* Predicate check S7 */
	while (is_queue_full(Q)) {
		printf("Thread %s blocks itself, Q is already Full\n", th_name);
		pthread_cond_wait(&Q->q_cv, &Q->q_mutex);
		printf("Thread %s wakes up, checking the Queue status again\n", th_name);
	}

	/*  PRoducer must start pushing elements in empty Queue only */
	assert(is_queue_empty(Q));

	int i;
	while(!is_queue_full(Q)) { /* Predicate check S7 */
		i = new_int();
		printf("Thread %s produces new integer %d\n", th_name, i);
		enqueue(Q, (void *)i); /* Dont ask me why I is type casted into void !, ok, you can ask me :p */
		printf("Thread %s pushed integer %d in Queue, Queue size = %d\n", th_name, i, Q->count);
		sleep(1);
	}
	
	printf("Thread %s Filled up the Queue, signalling and releasing lock\n", th_name);
	pthread_cond_signal(&Q->q_cv); /* S8 */
	pthread_mutex_unlock(&Q->q_mutex); /* S9 */
	return NULL;
}

static void *
cons_fn(void *arg) {

	char *th_name = (char *)arg;

	printf("Thread %s waiting to lock the Queue\n", th_name);
	pthread_mutex_lock(&Q->q_mutex); /* S1 */
	printf("Thread %s locks the Queue\n", th_name);

	/* Predicate check S2 */
	while (is_queue_empty(Q)) {
		printf("Thread %s blocks itself, Q is already empty\n", th_name);
		pthread_cond_wait(&Q->q_cv, &Q->q_mutex); /* S3 */
		printf("Thread %s wakes up, checking the Queue status again\n", th_name);
	}

	/*  Consumer must start consuming elements from Full Queue only */
	assert(is_queue_full(Q));

	/* S4 begin*/
	int i;
	while(!is_queue_empty(Q)) {
		i = (int)deque(Q);
		printf("Thread %s consumes an integer %d, Queue size = %d\n",
				th_name, i, Q->count);
		sleep(1);
	}
	/* send signal to Producer thread waiting on Queue */
	printf("Thread %s Drains the entire Queue, sending signal to Blocking Threads",
		th_name);
	pthread_cond_signal(&Q->q_cv);
	/*  S4 end*/
	
	printf("Thread %s releasing lock\n", th_name);
	pthread_mutex_unlock(&Q->q_mutex);	/* Step S5 */
	return NULL;
}

int
main(int argc, char **argv) {

	/* initialize the Queue and its Mutex + CV */
	Q = initQ();
	
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	pthread_t prod_th1, prod_th2; /*  Two producer threads */
	pthread_t cons_th1, cons_th2; /*  Two consumer threads */

	pthread_create(
				&prod_th1, &attr, prod_fn, (void *)prod1);
	
	pthread_create(
				&prod_th2, &attr, prod_fn, (void *)prod2);
	
	pthread_create(
				&cons_th1, &attr, cons_fn, (void *)cons1);
	
	pthread_create(
				&cons_th2, &attr, cons_fn, (void *)cons2);

	pthread_exit(0);
	return 0;
}

