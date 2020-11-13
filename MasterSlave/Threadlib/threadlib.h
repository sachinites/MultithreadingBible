/*
 * =====================================================================================
 *
 *       Filename:  threadlib.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  11/02/2020 01:15:21 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ABHISHEK SAGAR (), sachinites@gmail.com
 *   Organization:  Juniper Networks
 *
 * =====================================================================================
 */

#ifndef __THREAD_LIB__
#define __THREAD_LIB__

#include <stdbool.h>
#include <pthread.h>

typedef struct thread_{

    char name[32];
    pthread_t thread;
    void *(*thread_fn)(void *);
    pthread_cond_t cond_var;
    pthread_attr_t attributes;
	bool block_status;
} thread_t;

thread_t *
create_thread(thread_t *thread, char *name);

void
run_thread(thread_t *thread, void *(*thread_fn)(void *), void *arg);


#endif /* __THREAD_LIB__  */
