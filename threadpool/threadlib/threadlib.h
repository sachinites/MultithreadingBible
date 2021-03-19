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

typedef struct thread_fn_wrapper_ {
    void *(*thread_actual_fn)(void *);
    void *actual_arg;
    void (*thread_end_fn)(thread_pool_t *, thread_t *);
    thread_pool_t *thread_pool;
    thread_t *thread;
} thread_fn_wrapper_t;

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
