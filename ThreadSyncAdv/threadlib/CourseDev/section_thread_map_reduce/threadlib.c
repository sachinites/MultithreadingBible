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
#include <assert.h>
#include "gluethread/glthread.h"
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

void
thread_free(thread_t *thread) {

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


void
thread_pool_init (thread_pool_t *th_pool ) {

    init_glthread(&th_pool->pool_head);
    pthread_mutex_init(&th_pool->mutex, NULL);
}

void
thread_pool_insert_new_thread (thread_pool_t *th_pool, thread_t *thread) {

    pthread_mutex_lock(&th_pool->mutex);

    assert (IS_GLTHREAD_LIST_EMPTY(&thread->wait_glue));
    assert(thread->thread_fn == NULL);

    glthread_add_next(&th_pool->pool_head,
            &thread->wait_glue);

    pthread_mutex_unlock(&th_pool->mutex);
}

thread_t *
thread_pool_get_thread (thread_pool_t *th_pool) {
    
    thread_t *thread = NULL;
    glthread_t *glthread = NULL;

    pthread_mutex_lock(&th_pool->mutex);
    glthread = dequeue_glthread_first(&th_pool->pool_head);
    if (!glthread) {
        pthread_mutex_unlock(&th_pool->mutex);
        return NULL;
    }
    thread = wait_glue_to_thread(glthread);
    pthread_mutex_unlock(&th_pool->mutex);
    return thread;
}


static void
thread_pool_run_thread (thread_t *thread) {

    assert (IS_GLTHREAD_LIST_EMPTY(&thread->wait_glue));

    if (!thread->thread_created) {
        thread_run (thread, thread->thread_fn, thread->arg);
    }
    else {
        /*  If the thread is already created, it means the thread is
         *  in blocked state as it has completed its previous task. So
         *  resume the thread execution again
         *                                   */
        pthread_cond_signal(&thread->cv);
    }
}


static void
thread_pool_thread_stage3_fn(thread_pool_t *th_pool,
        thread_t *thread) {

    pthread_mutex_lock(&th_pool->mutex);

    glthread_add_next (&th_pool->pool_head,
                       &thread->wait_glue);

    if (thread->semaphore) {

        sem_post(thread->semaphore);
    }

    /*  Rest in peace again in thread pool after completing the task*/
    pthread_cond_wait(&thread->cv, &th_pool->mutex);
    pthread_mutex_unlock(&th_pool->mutex);
}

static void *
thread_fn_execute_stage2_and_stage3 (void *arg)  {

    thread_execution_data_t *thread_execution_data =
        (thread_execution_data_t *)arg;

    while ( 1 ) {
        /*  Stage 2 : User defined function with user defined argument*/
        thread_execution_data->thread_stage2_fn (thread_execution_data->stage2_arg);
        /*   Stage 3 : Queue the thread in thread pool and block it*/
        thread_execution_data->thread_stage3_fn (thread_execution_data->thread_pool, 
                thread_execution_data->thread);
    }
}

void
thread_pool_dispatch_thread (thread_pool_t *th_pool,     
                             void *(*thread_fn)(void *),
                             void *arg, bool block_caller) {

    /*  get the thread from the thread pool*/
    thread_t *thread = thread_pool_get_thread (th_pool);

    if (!thread) {
        return;
    }

    if (block_caller && !thread->semaphore) {

        thread->semaphore = calloc(1, sizeof(sem_t));
        sem_init(thread->semaphore, 0, 0);
    }

    thread_execution_data_t *thread_execution_data =
        (thread_execution_data_t *)(thread->arg);

    if (thread_execution_data == NULL ) {
        /*  In this data structure, we would wrap up all the information
         *  which thread needs to execute stage 2 ans stage 3
         **/
        thread_execution_data = calloc (1, sizeof(thread_execution_data_t));
    }

    /*  Setup Stage 2 in which thread would do assigned task*/
    thread_execution_data->thread_stage2_fn = thread_fn;
    thread_execution_data->stage2_arg = arg;

    /*  Setup Stage 3 in which thread would park itself in thread pool */
    thread_execution_data->thread_stage3_fn = thread_pool_thread_stage3_fn;
    thread_execution_data->thread_pool = th_pool;
    thread_execution_data->thread = thread;

    /*  Assign the aggregate work to the thread to perform i.e. Stage 2 followed
     *  by stage 3 */
    thread->thread_fn = thread_fn_execute_stage2_and_stage3;
    thread->arg = (void *)thread_execution_data;

    /*  Fire the thread now */
    thread_pool_run_thread (thread);

    if (block_caller) {
        /*  Wait for the thread to finish the Stage 2 and Stage 3 work*/
        sem_wait(thread->semaphore);
        /*  Caller is notified , destory the semaphore */
        sem_destroy(thread->semaphore);
        free(thread->semaphore);
        thread->semaphore = NULL;
    }
}


/* Implement Wait Queues APIs */

void
wait_queue_init (wait_queue_t * wq) {

    wq->thread_wait_count = 0;
    pthread_cond_init (&wq->cv, NULL);
    wq->appln_mutex = NULL;
}

/* All the majic of the Wait-Queue Thread-Synch Data structure lies in this
API call. ISt arg is a wait-queue, 2nd arg is a ptr to the application fn
the result (bool result) of which decides whether the calling thread need
to block on wait_queue or not. The 3rd param is the argument to a fn */
thread_t *
wait_queue_test_and_wait (wait_queue_t *wq,
                      wait_queue_condn_fn wait_queue_condn_fn_cb,
                      void *arg ) {
          
    bool should_block;
    pthread_mutex_t *locked_appln_mutex = NULL;

/* Invoke the application fn to decide whether the calling thread
  needs to be blocked. This fn must lock the application mutex, and test
  the appln specific condn and return true or false without unlocking
  the mutex */

    should_block = wait_queue_condn_fn_cb (arg, 
            &locked_appln_mutex);

    wq->appln_mutex = locked_appln_mutex; 

/* Conventional While loop which acts on predicate, and accordingly block
  the calling thread by invoking pthread_cond_wait*/
    while (should_block)
    {
        wq->thread_wait_count++;
        pthread_cond_wait (&wq->cv, wq->appln_mutex);
        wq->thread_wait_count--;
        /* The thread wakes up, retest the predicate again to 
         handle spurious wake up. Not that, appln need not test the
         block condition by locking the mutex this time since mutex is
         already locked in yhe first invocation of wait_queue_block_fn_cb()
         Hence Pass NULL as 2nd arg which hints the application that it has
         to test the predicate without locking any mutex */
        should_block = wait_queue_condn_fn_cb (arg, NULL);
    }
}

void
wait_queue_signal (wait_queue_t *wq, bool lock_mutex) {

    if (!wq->appln_mutex) return;

    if (lock_mutex) pthread_mutex_lock(wq->appln_mutex);

    if (!wq->thread_wait_count) {
        if (lock_mutex) pthread_mutex_unlock(wq->appln_mutex);
        return;
    }

    pthread_cond_signal (&wq->cv);

    if (lock_mutex) pthread_mutex_unlock(wq->appln_mutex);
}


void
wait_queue_broadcast (wait_queue_t *wq, bool lock_mutex) {

    if (!wq->appln_mutex) return;

    if (lock_mutex) pthread_mutex_lock(wq->appln_mutex);

    if (!wq->thread_wait_count) {
        if (lock_mutex) pthread_mutex_unlock(wq->appln_mutex);
        return;
    }

    pthread_cond_broadcast (&wq->cv);

    if (lock_mutex) pthread_mutex_unlock(wq->appln_mutex);
}


void
wait_queue_destroy (wait_queue_t *wq) {

    pthread_cond_destroy(&wq->cv);
    wq->appln_mutex = NULL;
}

/* 
 ********************************************
   Map-Reduce Implementation Starts Here 
 *********************************************
*/

map_reduce_t *
map_reduce_init(uint8_t n_mappers) {

    map_reduce_t *mr = calloc(1, sizeof(map_reduce_t) );
    mr->n_mappers = n_mappers;
    mr->mappers_in_progress = n_mappers;
    pthread_mutex_init(&mr->mr_mutex, NULL);
    wait_queue_init(&mr->wq_until_all_mappers_finish);
    sem_init(&mr->reducer_finished_semaphore, 0, 0);
    return mr;
}

void
map_reduce_set_data_splitter(map_reduce_t *mr, 
                                                 void (*input_data_splitter)(mr_iovec_t *, mr_iovec_t **, int)) {
                                                    
    mr->input_data_splitter = input_data_splitter;
}

void
map_reduce_set_app_data(map_reduce_t *mr, mr_iovec_t *app_data) {

    mr->app_data = app_data;
}

void
map_reduce_set_mapper_fn (map_reduce_t *mr,
                                             void (*mapper_fn)(thread_t *, mr_iovec_t *, mr_iovec_t **)) {

    mr->mapper_fn = mapper_fn;
}

void
map_reduce_set_mapper_input(map_reduce_t *mr, 
                                                    int index, mr_iovec_t *input) {

    assert(!mr->mapper_input_array[index] );
    mr->mapper_input_array[index] = input;
}

void
map_reduce_set_reducer_fn (map_reduce_t *mr,
                                             void (*reducer_fn)(thread_t *, mr_iovec_t **, int, mr_iovec_t *)) {

    mr->reducer_fn = reducer_fn;
}

void
map_reduce_set_reducer_output_reader (map_reduce_t *mr,
                                    void (*reducer_output_reader)(mr_iovec_t *iovec)) {

    mr->reducer_output_reader = reducer_output_reader;
}

static bool
 wq_is_any_mapper_in_progress(void *arg, pthread_mutex_t **mutex)  {

     map_reduce_t *mr = (map_reduce_t *)arg;

    if (mutex) {
        pthread_mutex_lock(&mr->mr_mutex);
        *mutex = &mr->mr_mutex;
    }

    return mr->mappers_in_progress ? true : false;
 }

 static void
 map_reduce_cleanup(map_reduce_t *mr) {

    int i = 0;

     for (i = 0; i < mr->n_mappers; i++) {

         if (mr->mapper_input_array [i] &&
                mr->mapper_input_array_cleanup ) {
             mr->mapper_input_array_cleanup(mr->mapper_input_array[i]);
             mr->mapper_input_array[i] = NULL;
         }
     }

    for (i = 0; i < mr->n_mappers; i++) {

         if (mr->mapper_result_array[i] &&
                mr->mapper_output_array_cleanup ) {
             mr->mapper_output_array_cleanup(mr->mapper_result_array[i]);
             mr->mapper_result_array[i] = NULL;
         }
     }

     for (i = 0; i < mr->n_mappers; i++){

         if (mr->mapper_thread_array[i] ) { 
             thread_free(mr->mapper_thread_array[i]);
             mr->mapper_thread_array[i] = NULL;
         }
     }

     if (mr->reducer_result.data_len && mr->reducer_output_cleanup) {
         mr->reducer_output_cleanup(&mr->reducer_result);
         mr->reducer_result.base = NULL;
         mr->reducer_result.data_len = 0;
     }

     if (mr->reducer_thread) {
         thread_free(mr->reducer_thread);
         mr->reducer_thread = NULL;
     }
 }

typedef struct mapper_data_ {

    int mapper_index;
    thread_t *mapper_thread;
    map_reduce_t *mr;
} mapper_data_t;

static void *
mapper_wrapper(void *arg) {

    thread_t *thread;
    map_reduce_t *mr;
    mapper_data_t *map_data = (mapper_data_t *)arg;
    thread = map_data->mapper_thread;
    mr = map_data->mr;
    printf("Mapper %s Start\n", thread->name);
    map_data->mr->mapper_fn(
                thread, mr->mapper_input_array[map_data->mapper_index],
                &mr->mapper_result_array[map_data->mapper_index] );
    pthread_mutex_lock(&mr->mr_mutex);
    mr->mappers_in_progress--;
    printf("Mapper %s Done\n", thread->name);
    wait_queue_broadcast(&mr->wq_until_all_mappers_finish, false);
    pthread_mutex_unlock(&mr->mr_mutex);
    free(map_data);
}

static void *
reducer_wrapper(void *arg) {

    map_reduce_t *mr = (map_reduce_t *)arg;
    
    pthread_mutex_lock(&mr->mr_mutex);
    mr->is_reducer_in_progress = true;
    pthread_mutex_unlock(&mr->mr_mutex);  
    printf("Reducer Started\n");

    mr->reducer_fn( mr->reducer_thread, 
                                (void *)mr->mapper_result_array, 
                                mr->n_mappers, 
                                &mr->reducer_result);

    printf("Reducer Done\n");
    pthread_mutex_lock(&mr->mr_mutex);
    mr->is_reducer_in_progress = false;
    pthread_mutex_unlock(&mr->mr_mutex);  
    sem_post(&mr->reducer_finished_semaphore);
}

void
map_reduce_start (map_reduce_t *mr) {

    int i;
    mapper_data_t *map_data;
    char thread_name[32];

    assert(mr->mappers_in_progress == mr->n_mappers);
    assert(mr->mapper_fn);

    mr->input_data_splitter(mr->app_data,
                                            mr->mapper_input_array,
                                            mr->n_mappers);

    mr->mappers_in_progress = mr->n_mappers;

    for (i = 0; i < mr->n_mappers; i++) {

        assert(!mr->mapper_thread_array[i]);
        sprintf(thread_name, "Mapper_%d", i);
        mr->mapper_thread_array[i] = thread_create(0, thread_name);
        map_data = calloc(1, sizeof (mapper_data_t));
        map_data->mapper_index = i;
        map_data->mapper_thread =  mr->mapper_thread_array[i];
        map_data->mr = mr;
        thread_run(mr->mapper_thread_array[i], 
                            mapper_wrapper, 
                            (void *)map_data);
    }

    wait_queue_test_and_wait(&mr->wq_until_all_mappers_finish,
                                                wq_is_any_mapper_in_progress,  (void *)mr);

    printf("Mapper Signalled, Mappers in progress = %d\n", mr->mappers_in_progress);
    assert(!mr->mappers_in_progress);
    pthread_mutex_unlock(mr->wq_until_all_mappers_finish.appln_mutex);
    
    /* Now launch Reducer fn */
    mr->reducer_thread = thread_create(0, "Reducer");
    thread_run(mr->reducer_thread,
                            reducer_wrapper, 
                            (void *) mr );

    sem_wait(&mr->reducer_finished_semaphore);

    /* output reader is optional */
    if (mr->reducer_output_reader) {
        mr->reducer_output_reader(&mr->reducer_result);
    }
    map_reduce_cleanup(mr);
}

bool
map_reduce_is_in_progress(map_reduce_t *mr) {

    bool is_in_progress;

    pthread_mutex_lock(&mr->mr_mutex);
    
    is_in_progress = mr->mappers_in_progress == 0 ? false : true;
    if (is_in_progress) goto done;
    is_in_progress = mr->is_reducer_in_progress ? true : false;
    
    done:
    pthread_mutex_unlock(&mr->mr_mutex);
    return is_in_progress;
}

/* 
 ********************************************
   Map-Reduce Implementation Ends Here 
 *********************************************
*/