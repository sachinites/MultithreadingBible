/*
 * =====================================================================================
 *
 *       Filename:  threadlib.c
 *
 *    Description: This file represents the thread library created over POSIX library 
 *
 *        Version:  1.0
 *        Created:  03/28/2021 07:14:35 AM
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
#include "threadlib.h"

thread_t *
thread_create(thread_t *thread, char *name) {
    
    if (!thread) {
        thread = calloc (1, sizeof(thread_t));
    }

    strncpy(thread->name, name, sizeof(thread->name));
    thread->thread_created = false;
    thread->arg = NULL;
    thread->thread_fn = NULL;
    pthread_attr_init(&thread->attributes);
    return thread;
}

void
thread_run(thread_t *thread, void *(*thread_fn)(void *), void *arg) {

    thread->thread_fn = thread_fn;
    thread->arg = arg;
    thread->thread_created = true;
    pthread_create(&thread->thread,
            &thread->attributes,
            thread_fn,
            arg);
}

void
thread_set_thread_attribute_joinable_or_detached(
                    thread_t *thread, bool joinable) {

    pthread_attr_setdetachstate(&thread->attributes,
            joinable ? PTHREAD_CREATE_JOINABLE : PTHREAD_CREATE_DETACHED);
}
