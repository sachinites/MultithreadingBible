/*
 * =====================================================================================
 *
 *       Filename:  joinable_example.c
 *
 *    Description: This file demonstrates the use of POSIX threads joinable API 
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
 * gcc -g -c joinable_example.c -o joinable_example.o
 * gcc -g joinable_example.o -o joinable_example.exe -lpthread
 * Run : ./joinable_example.exe
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> /* For working with POSIX threads*/
#include <unistd.h>  /* For pause() and sleep() */
#include <errno.h>	 /* For using Global variable errno */

/* A thread callback fn must have following prototypes 
 * void *(*thread_fn)(void *)
 * */
static void *
thread_fn_callback(void *arg) {

	int th_id = *(int *)arg ;
	int rc = 0 ;

	free(arg);

	while(rc != th_id) {

		printf("Thread %d doing some work\n", th_id);
		sleep(1);
		rc++;
	}

	int *result = calloc(1, sizeof(int));
	*result = th_id * th_id;
	return (void *)result;
}


void
thread_create(pthread_t *pthread_handle, int th_id) {

	pthread_attr_t attr;
	
	int *_th_id = calloc(1, sizeof(int));
	*_th_id = th_id;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE /* PTHREAD_CREATE_DETACHED  */);
	
	int rc = pthread_create(pthread_handle, 
				   &attr,
				   thread_fn_callback,
				   (void *)_th_id);
	if(rc != 0) {

		printf("Error occurred, thread could not be created, errno = %d\n", rc);
		exit(0);
	}
}

pthread_t pthread2;
pthread_t pthread3;

int
main(int argc, char **argv){

	void *thread_result2 = NULL;
	void *thread_result3 = NULL;

	thread_create(&pthread2, 2);
	
	thread_create(&pthread3, 10);

	printf("main fn blocked on pthread join for thread with th_id = 2\n");
	pthread_join(pthread2, &thread_result2);
	if(thread_result2) {
		printf("Return result from thread 2 = %d\n", *(int *)thread_result2);
		free(thread_result2);
		thread_result2 = NULL;
	}	

	printf("main fn blocked on pthread join for thread with th_id = 3\n");
	pthread_join(pthread3, &thread_result3);
	if(thread_result3) {
		printf("Return result from thread 3 = %d\n", *(int *)thread_result3);
		free(thread_result3);
		thread_result3 = NULL;
	}	

	return 0;
}

