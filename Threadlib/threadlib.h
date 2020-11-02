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
#include "../gluethread/glthread.h"

typedef enum{

    THREAD_READER,
    THREAD_WRITER
} thread_op_type_t;

typedef struct thread_{

    char name[32];
    pthread_t thread;
    glthread_t wait_glue;
    void *(*thread_fn)(void *);
    pthread_cond_t cond_var;
    pthread_attr_t attributes;
    thread_op_type_t thread_op;
} thread_t;
GLTHREAD_TO_STRUCT(wait_glue_to_thread,
        thread_t, wait_glue);

thread_t *
create_thread(thread_t *thread, char *name,
				thread_op_type_t thread_op);

void
run_thread(thread_t *thread, void *(*thread_fn)(void *), void *arg);


#endif /* __THREAD_LIB__  */
