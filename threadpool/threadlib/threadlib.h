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

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include "../gluethread/glthread.h"

typedef enum{

    THREAD_READER,
    THREAD_WRITER
} thread_op_type_t;

typedef struct thread_{

    char name[32];
    bool thread_created;
    pthread_t thread;
    void *arg;
    sem_t *caller_semaphore;
    void *(*thread_fn)(void *);
    pthread_cond_t cv;
    pthread_attr_t attributes;
    thread_op_type_t thread_op;
    glthread_t thread_pool_glue;
} thread_t;
GLTHREAD_TO_STRUCT(thread_pool_glue_to_thread,
        thread_t, thread_pool_glue);

thread_t *
create_thread(thread_t *thread, char *name,
				thread_op_type_t thread_op);

void
run_thread(thread_t *thread, void *(*thread_fn)(void *), void *arg);


/* Thread Pool */
typedef struct thread_pool_ {
  
  glthread_t pool_head;
  int (*comp_fn)(void *, void *);
  pthread_mutex_t mutex;
} thread_pool_t;

typedef struct thread_execution_data_ {
    /* Actual user defined work to be done by the thread*/
    void *(*thread_stage2_fn)(void *);
    /* Actual Input to be provided to the thread */
    void *stage2_arg;
    /* The fn executed by the thread when thread has finished its
       user defined work. This fn queues up the thread back in thread
       pool and optionally notifies the parent thread if required*/
    void (*thread_stage3_fn)(thread_pool_t *, thread_t *);
    /* Below two are Arguments for stage 3 function
    /* Thread pool to which the thread need to rest in peace after
       it has accomplished its task*/
    thread_pool_t *thread_pool;
    /* Data structure representing the thread*/
    thread_t *thread;
} thread_execution_data_t;

void
thread_pool_insert_new_thread(thread_pool_t *th_pool, thread_t *thread);

thread_t *
thread_pool_get_thread(thread_pool_t *th_pool);

void
thread_pool_init(thread_pool_t *th_pool, int (*comp_fn)(void *, void *));

void
thread_pool_dispatch_thread (thread_pool_t *th_pool,
                            void *(*thread_fn)(void *),
                            void *arg,
                            bool block_caller);

#endif /* __THREAD_LIB__  */















