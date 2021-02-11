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
are created in JOINABLE Mode.
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

	/* **************** 
	 * Join Telegram grp for QnA, Career Guidance,
	 * Dicount/Free Courses, Career Discussions etc.
 	 * Grp ID : telecsepracticals
 	 */

	/* Code Producer Logic here */
	return NULL;
}

static void *
cons_fn(void *arg) {

	char *th_name = (char *)arg;
	
	/* **************** 
	 * Join Telegram grp for QnA, Career Guidance,
	 * Dicount/Free Courses, Career Discussions etc.
 	 * Grp ID : telecsepracticals
 	 */

	/* Code Consumer Logic here */
	return NULL;
}

int
main(int argc, char **argv) {

	/* initialize the Queue and its Mutex + CV */
	Q = initQ();
	
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

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

	pthread_join(prod_th1, 0);
	pthread_join(prod_th2, 0);
	pthread_join(cons_th1, 0);
	pthread_join(cons_th2, 0);

	printf("Program Finished\n");
	pthread_exit(0);
	return 0;
}

