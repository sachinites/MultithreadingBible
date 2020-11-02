/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  11/02/2020 01:37:15 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ABHISHEK SAGAR (), sachinites@gmail.com
 *   Organization:  Juniper Networks
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "../Threadlib/threadlib.h"
#include "monitor.h"

monitor_t *mon;

void *
thread_fn(void *arg) {

	thread_t *thread = (thread_t *)arg;

	while(1){
	
		monitor_request_access_permission(
			mon, thread);

		printf("Thread %s Accessing Resource\n",
			thread->name);
	
		//sleep(1);

		printf("Thread %s Done with the Resource\n",
			thread->name);

		monitor_inform_resource_released(
			mon, thread);
	}
}


int
main(int argc, char **argv) {

	mon = init_monitor(0, "RT_TABLE");
	
	thread_t *thread1 = create_thread( 0, "Reader1",
						THREAD_READER);
	run_thread(thread1, thread_fn, thread1);
	
	thread_t *thread2 = create_thread( 0, "Reader2",
						THREAD_READER);
	run_thread(thread2, thread_fn, thread2);
	
	thread_t *thread3 = create_thread( 0, "Writer1",
						THREAD_WRITER);
	run_thread(thread3, thread_fn, thread3);
	
	thread_t *thread4 = create_thread( 0, "Writer2",
						THREAD_WRITER);
	run_thread(thread4, thread_fn, thread4);

	pause();
	return 0;
}
