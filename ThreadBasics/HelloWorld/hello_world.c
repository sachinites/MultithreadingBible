/*
 * =====================================================================================
 *
 *       Filename:  hello_world.c
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
 * gcc -g -c hello_world.c -o hello_world.o
 * gcc -g hello_world.o -o hello_world.exe -lpthread
 * Run : ./hello_world.exe
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

	char *input = (char *)arg;

	while(1) {	
		printf("input string = %s\n", input);
		sleep(1);
	}
}

void
thread1_create() {

	/* opaque object, dont bother about its internal
	 * members */
	pthread_t pthread1;

	/* Take some argument to be passed to the thread fn,
 	 * Look after that you always paas the persistent memory
 	 * as an argument to the thread, do not pass caller's 
 	 * local variables Or stack Memory*/	
	static char *thread_input1 = "I am thread no 1";

	/* Return 0 on success, otherwise returns errorcode */
	int rc = pthread_create(&pthread1, 
				   NULL, 
				   thread_fn_callback,
				   (void *)thread_input1);
	if(rc != 0) {

		printf("Error occurred, thread could not be created, errno = %d\n", rc);
		exit(0);
	}
}

int
main(int argc, char **argv){

	thread1_create();
	printf("main fn paused\n");
	pause();
	return 0;
}

