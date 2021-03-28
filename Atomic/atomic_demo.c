/*
 * =====================================================================================
 *
 *       Filename:  atomic_demo.c
 *
 *    Description: This file demonstrates the use of POSIX threads - A hello world program 
 *
 *        Version:  1.0
 *        Created:  11/03/2020 07:50:04 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ABHISHEK SAGAR (), sachinites@gmail.com
 *   Organization:  Juniper Networks
 *
 * =====================================================================================
 */

/*
 * compile using :
 * gcc -g -c atomic_demo.c -o atomic_demo.o
 * gcc -g atomic_demo.o -o atomic_demo.exe -lpthread
 * Run : ./atomic_demo.exe
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> /* For working with POSIX threads*/
#include <unistd.h>  /* For pause() and sleep() */
#include <errno.h>	 /* For using Global variable errno */
#include <assert.h>
#include <stdatomic.h>

int arr[] = { 1, 2, 3, 4, 5};

static atomic_flag at_f = ATOMIC_FLAG_INIT;

void
print_array(){

	int i = 0;
	int arr_size = sizeof(arr)/sizeof(arr[0]);

	for( ; i < arr_size -1; i++){
		
		printf("%d ", arr[i]);
	}
	printf("%d\n", arr[i]);
}

/* A thread callback fn must have following prototypes 
 * void *(*thread_fn)(void *)
 * */
static void *
thread_fn_callback_sum(void *arg) {

	int i;
	int sum;
	
	int arr_size = sizeof(arr)/sizeof(arr[0]);

	do {
		sum = 0;
		i = 0;
                atomic_flag_test_and_set(&at_f);
		while(i < arr_size) {
			sum += arr[i];
			i++;
		}
		printf("sum = %d\n", sum);
		print_array();
		assert(sum == 15);
                atomic_flag_clear(&at_f);
	} while(1);
}

static void *
thread_fn_callback_swap(void *arg) {

	int temp;
	int arr_size = sizeof(arr)/sizeof(arr[0]);

	do {
                atomic_flag_test_and_set(&at_f);
		temp = arr[0];	
		arr[0] = arr[arr_size -1];
		arr[arr_size-1] = temp;	
		printf("swap :\n");
		print_array();
                atomic_flag_clear(&at_f);
	} while(1);
}

void
sum_thread_create() {

	/* opaque object, dont bother about its internal
	 * members */
	pthread_t pthread1;

	int rc = pthread_create(&pthread1, 
				   NULL, 
				   thread_fn_callback_sum,
				   NULL);
	if(rc != 0) {

		printf("Error occurred, thread could not be created, errno = %d\n", rc);
		exit(0);
	}
}

void
swap_thread_create() {

	/* opaque object, dont bother about its internal
	 * members */
	pthread_t pthread2;

	int rc = pthread_create(&pthread2, 
				   NULL, 
				   thread_fn_callback_swap,
				   NULL);
	if(rc != 0) {

		printf("Error occurred, thread could not be created, errno = %d\n", rc);
		exit(0);
	}
}
int
main(int argc, char **argv){

	sum_thread_create();
	swap_thread_create();

	/*  Allow main thread to exit, while
 	 *  other threads would keep running */
	pthread_exit(0);
	return 0;
}

