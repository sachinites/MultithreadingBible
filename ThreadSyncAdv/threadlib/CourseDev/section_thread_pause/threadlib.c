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
#include "bitsop.h"

thread_t *
thread_create(thread_t *thread, char *name) {
    
    if (!thread) {
        thread = calloc (1, sizeof(thread_t));
    }

    strncpy(thread->name, name, sizeof(thread->name));
    thread->thread_created = false;
    thread->arg = NULL;
    thread->thread_fn = NULL;
	thread->flags = 0;
	thread->thread_pause_fn = 0;
	thread->pause_arg = 0;
	pthread_mutex_init(&thread->state_mutex, NULL);
	pthread_cond_init(&thread->cv, NULL);
	pthread_attr_init(&thread->attributes);
    return thread;
}

void
thread_run(thread_t *thread, void *(*thread_fn)(void *), void *arg) {

    thread->thread_fn = thread_fn;
    thread->arg = arg;
    thread->thread_created = true;
	SET_BIT(thread->flags, THREAD_F_RUNNING);
    pthread_create(&thread->thread,
            &thread->attributes,
            thread_fn,
            arg);
}

void
thread_set_thread_attribute_joinable_or_detached(
                    thread_t *thread, bool joinable) {

    pthread_attr_setdetachstate(&thread->attributes,
            joinable ? PTHREAD_CREATE_JOINABLE :
            PTHREAD_CREATE_DETACHED);
}

/*
 * Thread pausing and resuming
 */

void
thread_set_pause_fn(thread_t *thread,
                    void *(*thread_pause_fn)(void *),
                    void *pause_arg) {

    thread->thread_pause_fn = thread_pause_fn;
    thread->pause_arg = pause_arg;
}

void
thread_pause(thread_t *thread) {

    pthread_mutex_lock(&thread->state_mutex);

    if (IS_BIT_SET(thread->flags, THREAD_F_RUNNING)) {

        SET_BIT(thread->flags, THREAD_F_MARKED_FOR_PAUSE);
    }
    pthread_mutex_unlock(&thread->state_mutex);
}

void
thread_resume(thread_t *thread) {

    pthread_mutex_lock(&thread->state_mutex);

    if (IS_BIT_SET(thread->flags, THREAD_F_PAUSED)) {
        pthread_cond_signal(&thread->cv);
    }
    pthread_mutex_unlock(&thread->state_mutex);
}

void
thread_test_and_pause(thread_t *thread) {

    pthread_mutex_lock(&thread->state_mutex);

    if (IS_BIT_SET(thread->flags, THREAD_F_MARKED_FOR_PAUSE)) {

        SET_BIT(thread->flags, THREAD_F_PAUSED);
        UNSET_BIT(thread->flags, THREAD_F_MARKED_FOR_PAUSE);
		UNSET_BIT(thread->flags, THREAD_F_RUNNING);
        pthread_cond_wait(&thread->cv, &thread->state_mutex);
		SET_BIT(thread->flags, THREAD_F_RUNNING);
                UNSET_BIT(thread->flags, THREAD_F_PAUSED);
		(thread->thread_pause_fn)(thread->pause_arg);
        pthread_mutex_unlock(&thread->state_mutex);
    }
    else {
        pthread_mutex_unlock(&thread->state_mutex);
    }
}

