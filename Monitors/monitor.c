/*
 * =====================================================================================
 *
 *       Filename:  monitor.c
 *
 *    Description: This file implementes routines for Monitors 
 *
 *        Version:  1.0
 *        Created:  11/02/2020 12:12:35 AM
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
#include <memory.h>
#include <assert.h>
#include "../Threadlib/threadlib.h"
#include "monitor.h"


monitor_t *
init_monitor(monitor_t *monitor,
			 char *resource_name){

	if(monitor == NULL) {
		monitor = calloc(1, sizeof(monitor_t));
	}

	strncpy(monitor->name, resource_name,
			sizeof(monitor->name));

	pthread_mutex_init(&monitor->monitor_talk_mutex, 0);
	init_glthread(&monitor->writer_thread_wait_q);
	init_glthread(&monitor->reader_thread_wait_q);
	init_glthread(&monitor->resource_using_threads_q);
	monitor->resource_status = MON_RES_AVAILABLE;
	return monitor;
}

static inline void
monitor_lock_monitor_talk_mutex(monitor_t *monitor) {

	printf("Monitor %s : Talk Mutex Locked\n", monitor->name);
	pthread_mutex_lock(&monitor->monitor_talk_mutex);
}

static inline void
monitor_unlock_monitor_talk_mutex(monitor_t *monitor) {

	printf("Monitor %s : Talk Mutex UnLocked\n", monitor->name);
	pthread_mutex_unlock(&monitor->monitor_talk_mutex);
}

static void
monitor_wait_list_client_thread(
	monitor_t *monitor,
	thread_t *requester_thread) {

	init_glthread(&requester_thread->wait_glue);
	
	if(requester_thread->thread_op == THREAD_READER) {

		printf("Thread %s Added to Reader Wait Q of Monitor %s\n",
				requester_thread->name,
				monitor->name);

		glthread_add_next(&monitor->reader_thread_wait_q,
						  &requester_thread->wait_glue);
	}
	else {

		printf("Thread %s Added to Writer Wait Q of Monitor %s\n",
				requester_thread->name,
				monitor->name);

		glthread_add_next(&monitor->writer_thread_wait_q,
						  &requester_thread->wait_glue);
	}

	printf("Thread %s blocked by Monitor %s\n",
			requester_thread->name,
			 monitor->name);	
	pthread_cond_wait(&requester_thread->cond_var,
					  &monitor->monitor_talk_mutex);
}


void
monitor_request_access_permission(
	monitor_t *monitor,
	thread_t *requester_thread) {

	monitor_lock_monitor_talk_mutex(monitor);

	printf("Thread %s(%d) requesting Monitor %s for Resource accesse\n",
			requester_thread->name,
			requester_thread->thread_op,
			monitor->name);

	if(monitor->resource_status == MON_RES_AVAILABLE) {

		monitor->resource_status = requester_thread->thread_op == THREAD_READER ?
				MON_RES_BUSY_BY_READER : MON_RES_BUSY_BY_WRITER;

		printf("Monitor %s resource available, Thread %s granted Access\n",
			monitor->name, requester_thread->name);
		glthread_add_next(&monitor->resource_using_threads_q, 
				&requester_thread->wait_glue);	
		monitor_unlock_monitor_talk_mutex(monitor);
		return;
	}
	else {

		if(requester_thread->thread_op == THREAD_READER &&
			monitor->resource_status == MON_RES_BUSY_BY_READER) {

			printf("Multiple Readers : Monitor %s resource available, "
					"Thread %s granted Access\n",
			monitor->name, requester_thread->name);	
			glthread_add_next(&monitor->resource_using_threads_q, 
				&requester_thread->wait_glue);	
			monitor_unlock_monitor_talk_mutex(monitor);
			return;
		}

		/* In any other case, the requester thread should be blocked */
		monitor_wait_list_client_thread(monitor, requester_thread);
		
		/* Do when client thread is unblocked */
		printf("Thread %s is resumed, and granted access to resource by Monitor %s\n",
				requester_thread->name,
				monitor->name);
		monitor->resource_status = requester_thread->thread_op == THREAD_READER ?
				MON_RES_BUSY_BY_READER : MON_RES_BUSY_BY_WRITER;
		remove_glthread(&requester_thread->wait_glue);
		monitor_unlock_monitor_talk_mutex(monitor);
		return;
	}
}

void
monitor_inform_resource_released(
	 monitor_t *monitor,
	 thread_t *requester_thread){

	thread_t *next_accessor_thread;
	glthread_t *next_accessor_thread_glue;

	monitor_lock_monitor_talk_mutex(monitor);

	printf("Thread %s(%d) informing Monitor %s for Resource release\n",
			requester_thread->name,
			requester_thread->thread_op,
			monitor->name);

	remove_glthread(&requester_thread->wait_glue);
	init_glthread(&requester_thread->wait_glue);

	if(!IS_GLTHREAD_LIST_EMPTY(&monitor->resource_using_threads_q)) {
		printf("Some Threads still using resource\n");
		monitor_unlock_monitor_talk_mutex(monitor);
		return;
	}
	
	if (IS_GLTHREAD_LIST_EMPTY(&monitor->reader_thread_wait_q)) {

		if(IS_GLTHREAD_LIST_EMPTY(&monitor->writer_thread_wait_q)) {

			/* There is no thread waiting for a resource */
			monitor->resource_status = MON_RES_AVAILABLE;
			printf("No More Threads in Wait Q;s of Monitor %s, "
				   "Res Marked Available\n", monitor->name);
			monitor_unlock_monitor_talk_mutex(monitor);
			return;
		}
		else {
			/* Some Writer thread is Waiting */
			next_accessor_thread_glue = dequeue_glthread_first(
				&monitor->writer_thread_wait_q);
			next_accessor_thread = 	wait_glue_to_thread(next_accessor_thread_glue);
			printf("Monitor %s Picks up next Writer Thread %s for Resource Access\n",
				monitor->name, next_accessor_thread->name);
			monitor->resource_status = MON_RES_BUSY_BY_WRITER;
			pthread_cond_signal(&next_accessor_thread->cond_var);
			monitor_unlock_monitor_talk_mutex(monitor);
			return;				
		}
	}
	else {
		/* One or more Reader threads are waiting */
		while(!IS_GLTHREAD_LIST_EMPTY(&monitor->reader_thread_wait_q)) {
			printf("Awakening all Readers\n");
			/*  Some Reader thread is Waiting */
			next_accessor_thread_glue = dequeue_glthread_first(
					&monitor->reader_thread_wait_q);
			next_accessor_thread =  wait_glue_to_thread(next_accessor_thread_glue);
			printf("Monitor %s Picks up next Reader Thread %s for Resource Access\n",
					monitor->name, next_accessor_thread->name);
			monitor->resource_status = MON_RES_BUSY_BY_READER;
			pthread_cond_signal(&next_accessor_thread->cond_var);
		}
		monitor_unlock_monitor_talk_mutex(monitor);
		return;
	}
}

