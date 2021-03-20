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
#include <assert.h>
#include <unistd.h>
#include "threadlib.h"


/* Fn to create and initialize a new thread Data structure
   When a new thread_t is created, it is just a data structure
   sitting in the memory, not an execution unit. The actual execution
   unit is thread_t->thread which we shall be creating using
   pthread_create( ) API later
 */
thread_t *
create_thread(thread_t *thread,
			  char *name,
			  thread_op_type_t thread_op) {

	if (!thread) {
		thread = calloc (1, sizeof(thread_t));
	}
    
    strncpy(thread->name, name, sizeof(thread->name));  
    thread->thread_created = false;
    thread->arg = NULL;
    thread->caller_semaphore = NULL;
    thread->thread_fn = NULL;
	pthread_cond_init(&thread->cv, 0);
	pthread_attr_init(&thread->attributes);
	thread->thread_op = thread_op;
    init_glthread(&thread->thread_pool_glue);
	return thread;
}

void
run_thread(thread_t *thread,
		   void *(*thread_fn)(void *),
		   void *arg){

	thread->thread_fn = thread_fn;
    thread->arg = arg;
	pthread_attr_init(&thread->attributes);
	pthread_attr_setdetachstate(&thread->attributes, PTHREAD_CREATE_JOINABLE);
    thread->thread_created = true;
	pthread_create(&thread->thread,
                   &thread->attributes,
				   thread_fn,
                   arg);
}

/* Thread Pool Implementation Starts from here*/

static void
thread_pool_run_thread (thread_t *thread) {
    /* 
       This fn assumes that thread has already been removed from
       thread pool
     */
    assert (IS_GLTHREAD_LIST_EMPTY(&thread->thread_pool_glue));
    
    if (!thread->thread_created) {
        run_thread(thread, thread->thread_fn, thread->arg);
    }
    else {
        /* If the thread is already created, it means the thread is
           in blocked state as it has completed its previous task. So
           resume the thread execution again
         */
        pthread_cond_signal(&thread->cv);
    }
}

static void
thread_pool_thread_stage3_fn(thread_pool_t *th_pool,
                          thread_t *thread) {
    
    pthread_mutex_lock(&th_pool->mutex);
    
    glthread_priority_insert (&th_pool->pool_head,
                              &thread->thread_pool_glue,
                              th_pool->comp_fn,
                              (size_t)&(((thread_t *)0)->thread_pool_glue));
    
    /* Tell the caller thread (which dispatched me from pool) that in
    am done */
    if (thread->caller_semaphore){
        sem_post(thread->caller_semaphore);
    }

    /* Rest in peace again in thread pool after completing the task*/
    pthread_cond_wait(&thread->cv, &th_pool->mutex);
    pthread_mutex_unlock(&th_pool->mutex);
}

static void *
thread_fn_execute_stage2_and_stage3(void *arg) {

    thread_execution_data_t *thread_execution_data =
            (thread_execution_data_t *)arg;
    
    while(true) {
         /* Stage 2 : USer defined function with user defined argument*/
        thread_execution_data->thread_stage2_fn (thread_execution_data->stage2_arg);
        /*  Stage 3 : Queue the thread in thread pool and block it*/
        thread_execution_data->thread_stage3_fn (thread_execution_data->thread_pool, 
                                          thread_execution_data->thread);
   }
}

/* This fn trigger the thread work cycle which involves :
    1. Stage 1 : Picking up the thread from thread pool
                 Set up the Zero semaphore (optionally) if parent thread needs to blocked
                    until the worker thread finishes the task
    2. Setup stage 2 and stage 3
        2.a : Stage 2 - Thread does the actual task assigned by the appln
        2.b : Stage 3 - Thread park itself back in thread pool and notify the
            appln thread (parent thread) if required
            
    thread_execution_data_t is a data structure which stores the functions (unit of work) along
    with the data (arguments) to be processed by worker thread in stage 2 and stage 3
*/
static void
thread_pool_thread_stage1_fn (thread_pool_t *th_pool,
                            void *(*thread_fn)(void *),
                            void *arg,
                            bool block_caller) {

    sem_t *sem0_1 = NULL;
    
    if (block_caller) {
        sem0_1 = calloc(1, sizeof(sem_t));
        sem_init(sem0_1, 0, 0);
    }
    
    /* get the thread from the thread pool*/
    thread_t *thread = thread_pool_get_thread (th_pool);
    
    if (!thread) {
        return;
    }
    
    /* Cache the semaphore in the thread itself, so that when
       thread finishes the stage 3, it can notify the parent thread
    */
    thread->caller_semaphore = sem0_1 ? sem0_1 : NULL;
    
    thread_execution_data_t *thread_execution_data = 
        (thread_execution_data_t *)(thread->arg);
    
     if (thread_execution_data == NULL ) {
         /* In this data structure, we would wrap up all the information
            which thread needs to execute stage 2 ans stage 3
         */
        thread_execution_data = calloc (1, sizeof(thread_execution_data_t));
     }
    
    /* Setup Stage 2 in which thread would do assigned task*/
    thread_execution_data->thread_stage2_fn = thread_fn;
    thread_execution_data->stage2_arg = arg;
    
    /* Setup Stage 3 in which thread would park itself in thread pool */
    thread_execution_data->thread_stage3_fn = thread_pool_thread_stage3_fn;
    thread_execution_data->thread_pool = th_pool;
    thread_execution_data->thread = thread;
    
    /* Assign the aggregate work to the thread to perform i.e. Stage 2 followed
       by stage 3 */
    thread->thread_fn = thread_fn_execute_stage2_and_stage3;
    thread->arg = (void *)thread_execution_data;
    
    /* Fire the thread now */
    thread_pool_run_thread (thread);
    
   if (block_caller) {
       /* Wait for the thread to finish the Stage 2 and Stage 3 work*/
       sem_wait(sem0_1);
       /* Caller is notified , destory the semaphore */
       sem_destroy(sem0_1);
       free(sem0_1);
       sem0_1 = NULL;
   }
}

void
thread_pool_dispatch_thread (thread_pool_t *th_pool,
                            void *(*thread_fn)(void *),
                            void *arg,
                            bool block_caller) {

    thread_pool_thread_stage1_fn (th_pool, thread_fn, arg, block_caller);
}
    
void
thread_pool_init(thread_pool_t *th_pool,
                 int (*comp_fn)(void *, void *)) {
    
    init_glthread(&th_pool->pool_head);
    th_pool->comp_fn = comp_fn;
    pthread_mutex_init(&th_pool->mutex, NULL);
}

void
thread_pool_insert_new_thread(thread_pool_t *th_pool,
                          thread_t *thread) {

   
    pthread_mutex_lock(&th_pool->mutex);
    
    assert (IS_GLTHREAD_LIST_EMPTY(&thread->thread_pool_glue));
    assert(thread->thread_fn == NULL);
    
    glthread_priority_insert (&th_pool->pool_head,
                              &thread->thread_pool_glue,
                              th_pool->comp_fn,
                              (size_t)&(((thread_t *)0)->thread_pool_glue));
                              
    pthread_mutex_unlock(&th_pool->mutex);
}

thread_t *
thread_pool_get_thread(thread_pool_t *th_pool) {

    thread_t *thread = NULL;
    glthread_t *glthread = NULL;
    
    pthread_mutex_lock(&th_pool->mutex);
    glthread = dequeue_glthread_first(&th_pool->pool_head);
    if (!glthread) {
        pthread_mutex_unlock(&th_pool->mutex);
        return NULL;
    }
    thread = thread_pool_glue_to_thread(glthread);
    pthread_mutex_unlock(&th_pool->mutex);
    return thread;
}

/* Main application using thread pool starts here */

/* Comparison fn to enqueue the thread in thread-pool. Returning -1 means
   newly added thread would queue up in the front of the thread pool list
*/
int
thread_pool_thread_insert_comp_fn(void *thread1, void *thread2){

    return -1;
}

void *
even_thread_work(void *arg) {

    int i;
    for (i = 0; i < 10; i+=2) {
        printf("even = %d\n", i);
        sleep(1);
    }
}

void *
odd_thread_work(void *arg) {

    int i;
    for (i = 1; i < 10; i+=2) {
        printf("odd = %d\n", i);
        sleep(1);
    }
}


int
main(int argc, char **argv) {

    /* Create and initialze a thread pool */
    thread_pool_t *th_pool = calloc(1, sizeof(thread_pool_t));
    thread_pool_init(th_pool, thread_pool_thread_insert_comp_fn);
    
    /* Create two threads (not execution units, just thread_t data structures) */
    thread_t *thread1 = create_thread(0, "even_thread", THREAD_WRITER);
    thread_t *thread2 = create_thread(0, "odd_thread", THREAD_WRITER);
    
    /* Insert both threads in thread pools*/
    thread_pool_insert_new_thread(th_pool, thread1);
    thread_pool_insert_new_thread(th_pool, thread2);
    
    thread_pool_dispatch_thread(th_pool, even_thread_work, 0, false);
    thread_pool_dispatch_thread(th_pool, odd_thread_work,  0, false);
    
    pthread_exit(0);
    return 0;
}






















