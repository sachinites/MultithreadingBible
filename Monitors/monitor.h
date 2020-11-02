/*
 * =====================================================================================
 *
 *       Filename:  monitor.h
 *
 *    Description: This file defines the data structures to implement Monitors 
 *
 *        Version:  1.0
 *        Created:  11/01/2020 11:46:29 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ABHISHEK SAGAR (), sachinites@gmail.com
 *   Organization:  Juniper Networks
 *
 * =====================================================================================
 */


#ifndef __MONITOR__
#define __MONITOR__

#include <stdbool.h>
#include <pthread.h>
#include "../gluethread/glthread.h"

typedef struct thread_ thread_t;

/* Monitor specific Data Structures and APIs */

typedef enum {

	MON_RES_AVAILABLE,
	MON_RES_BUSY_BY_READER,
	MON_RES_BUSY_BY_WRITER,
} resource_status_t;

typedef struct monitor_{

	/*  Name of the resource which is protected by this Monitor */
	char name[32];	

	/* Threads (clients) will talk to monitor in a Mutual Exclusion Way */
	pthread_mutex_t monitor_talk_mutex;

	/* List of writer threads  waiting on a resource*/
	glthread_t writer_thread_wait_q;

	/* List of Reader threads Waiting on a resource */
	glthread_t reader_thread_wait_q;

	/* List of Threads using the resource currently,
     * Multiple threads if Multiple Readers are accessing,
     * Only one thread in list of it is a Writer thread*/
	glthread_t resource_using_threads_q;

	/* Status of the resource */
	resource_status_t resource_status;
} monitor_t;

monitor_t *
init_monitor(monitor_t *monitor,
			 char *resource_name);

/* fn used by the client thread to request read/write access
 * on a resource. Fn returns if permission is granted,
 * else the fn is blocked and stay blocked until request
 * is granted to the calling thread*/
void
monitor_request_access_permission(
	monitor_t *monitor,
	thread_t *requester_thread);

/* fn used by the client thread to tell the monitor that
 * client is done with the resource. Thid fn do not blocks*/
void
monitor_inform_resource_released(
	monitor_t *monitor,
	thread_t *requester_thread);


#endif /*  __MONITOR__  */
