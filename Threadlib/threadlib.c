/*
 * =====================================================================================
 *
 *       Filename:  threadlib.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  11/02/2020 01:20:30 AM
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
#include <memory.h>
#include "threadlib.h"

thread_t *
create_thread(thread_t *thread,
			  char *name,
			  thread_op_type_t thread_op){

	if (!thread) {
		thread = malloc(sizeof(thread_t));
	}

	memset(thread, 0 , sizeof(thread_t));
	strncpy(thread->name, name, sizeof(thread->name));
	init_glthread(&thread->wait_glue);
	pthread_cond_init(&thread->cond_var, 0);
	pthread_attr_init(&thread->attributes);
	thread->thread_op = thread_op;
	return thread;
}

void
run_thread(thread_t *thread,
		   void *(*thread_fn)(void *),
		   void *arg){

	thread->thread_fn = thread_fn;
	pthread_attr_init(&thread->attributes);
	pthread_attr_setdetachstate(&thread->attributes, PTHREAD_CREATE_JOINABLE);
	pthread_create(&thread->thread, &thread->attributes,
				thread_fn, arg);
}

