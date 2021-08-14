/*
 * =====================================================================================
 *
 *       Filename:  threadlib.h
 *
 *    Description:  This file defines the commonly used data structures and routines for
 	for thread synchronization
 *
 *        Version:  1.0
 *        Created:  03/23/2021 01:20:30 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ABHISHEK SAGAR (), sachinites@gmail.com
 *   Organization:  Juniper Networks ( Apr 2017 - Mar 2021)
 *					Cisco (Mar 2021 - Present)
 *
 * =====================================================================================
 */

/*
  Visit : www.csepracticals.com for more courses and projects
  Join Telegram Grp : telecsepracticals
*/

#ifndef __THREAD_LIB__
#define __THREAD_LIB__

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <semaphore.h>

/* When the thread is running and doing its work as normal */
#define THREAD_F_RUNNING            (1 << 0)
/* When the thread has been marked to pause, but not paused yet */
#define THREAD_F_MARKED_FOR_PAUSE   (1 << 1)
/* When thread is blocked (paused) */
#define THREAD_F_PAUSED             (1 << 2)
/* When thread is blocked on CV for reason other than paused */
#define THREAD_F_BLOCKED            (1 << 3)

typedef struct thread_{
	/*name of the thread */
    char name[32];
	/* is execution unit has been created*/
    bool thread_created;
	/* pthread handle */
    pthread_t thread;
	/* thread fn arg */
    void *arg;
	/* thread fn */
    void *(*thread_fn)(void *);
    /* Fn to be invoked just before pauing the thread */
    void *(*thread_pause_fn)(void *);
    /* Arg to be supplied to pause fn */
    void *pause_arg;
    /* track thread state */
    uint32_t flags;
    /* update thread state mutually exclusively */
    pthread_mutex_t state_mutex;
    /* cv on which thread will block itself*/
    pthread_cond_t cv;
    /* thread Attributes */
    pthread_attr_t attributes;
    sem_t *semaphore;
    glthread_t wait_glue;
} thread_t;
GLTHREAD_TO_STRUCT(wait_glue_to_thread,
                thread_t, wait_glue);

thread_t *
thread_create(thread_t *thread, char *name);

void
thread_run(thread_t *thread, void *(*thread_fn)(void *), void *arg);

void
thread_set_thread_attribute_joinable_or_detached(
            thread_t *thread, bool joinable);


/* Thead pausing and resuming */

void
thread_set_pause_fn(thread_t *thread,
                    void *(*thread_pause_fn)(void *),
                    void *pause_arg);

void
thread_pause(thread_t *thread);

void
thread_resume(thread_t *thread);

void
thread_test_and_pause(thread_t *thread);

void
thread_free(thread_t *thread);

/* Thread Pool Begin */


typedef struct thread_pool_ {

  glthread_t pool_head;
  pthread_mutex_t mutex;
} thread_pool_t;


typedef struct thread_execution_data_ {

    void *(*thread_stage2_fn)(void *);
    void *stage2_arg;

    void (*thread_stage3_fn)(thread_pool_t *, thread_t *);
    thread_pool_t *thread_pool;
    thread_t *thread;

} thread_execution_data_t;


void
thread_pool_init (thread_pool_t *th_pool );

void
thread_pool_insert_new_thread (thread_pool_t *th_pool, thread_t *thread);

thread_t *
thread_pool_get_thread (thread_pool_t *th_pool);

void
thread_pool_dispatch_thread (thread_pool_t *th_pool,     
                            void *(*thread_fn)(void *),
                            void *arg, bool block_caller);



/* Wait Queues Implementation Starts here */

typedef struct wait_queue_ {

    /*  No of threads waiting in a wait-queue*/
    uint32_t thread_wait_count;

    /*  CV on which multiple threads in wait-queue are   blocked */
    pthread_cond_t cv;

    /*  Application owned Mutex cached by wait-queue */
    pthread_mutex_t *appln_mutex;

} wait_queue_t;


typedef bool (*wait_queue_condn_fn)
      (void *appln_arg, pthread_mutex_t **out_mutex);

void
wait_queue_init (wait_queue_t * wq);


thread_t *
wait_queue_test_and_wait (wait_queue_t *wq,
        wait_queue_condn_fn wait_queue_block_fn_cb,
        void *arg );


void
wait_queue_signal (wait_queue_t *wq, bool lock_mutex);


void
wait_queue_broadcast (wait_queue_t *wq, bool lock_mutex);


void
wait_queue_destroy (wait_queue_t *wq);


/* Wait Queues Implementation Ends here */




/* Map-Reduce Implementation Starts Here */

typedef struct mr_iovec_ {

    void *base;
    uint32_t data_len;
} mr_iovec_t;

#define MAP_REDUCE_MAX_MAPPER   8

typedef struct map_reduce_ {

    /* No of mappers */
    uint8_t n_mappers;
    
    /* Application Data */
    mr_iovec_t *app_data;
    /* Split input data, and put a chunk of them in 
        mapper_input_array */
    void (*input_data_splitter)(mr_iovec_t *, mr_iovec_t **, int);

    /* Mappers specific attributes */   

    /* Mapper function */
    void  (*mapper_fn)(thread_t *, mr_iovec_t *, mr_iovec_t **);
    /* No of mappers in progress */
    uint8_t mappers_in_progress;
    /* Mutex to update map-reduce state*/
    pthread_mutex_t mr_mutex;
    /* Wait Queue until all mapers have finished */
    wait_queue_t wq_until_all_mappers_finish;
    /* Mapper input input array */
    mr_iovec_t *mapper_input_array[MAP_REDUCE_MAX_MAPPER];
    /* Mapper result array */
    mr_iovec_t *mapper_result_array[MAP_REDUCE_MAX_MAPPER];
    /* Mapper fn array */
    thread_t *mapper_thread_array[MAP_REDUCE_MAX_MAPPER];

    /* Reducer Specific Attributes */

    /* reducer Thread */
    thread_t *reducer_thread;
    /* Final reducer result */
    mr_iovec_t reducer_result;
        /* Reducer function */
    void (*reducer_fn)(thread_t *, mr_iovec_t **, int, mr_iovec_t *);
    /* Zero semaphore to wait until reducer is finished */
    sem_t reducer_finished_semaphore;
    /* True if reducer is in progress */
    bool is_reducer_in_progress;
    /* Reducer ourput reading function */
    void (*reducer_output_reader)(mr_iovec_t *iovec);

    /* clean up functions */
    void (*mapper_input_array_cleanup)(mr_iovec_t *);
    void (*mapper_output_array_cleanup)(mr_iovec_t *);
    void (*reducer_output_cleanup)(mr_iovec_t *);
} map_reduce_t;

/* Map-Reduce Implementation Ends Here */

map_reduce_t *
map_reduce_init(uint8_t n_mappers);

void
map_reduce_set_data_splitter(map_reduce_t *mr, 
                                                 void (*input_data_splitter)(mr_iovec_t *, mr_iovec_t **, int));

void
map_reduce_set_app_data(map_reduce_t *mr, mr_iovec_t *app_data);

void
map_reduce_set_mapper_fn (map_reduce_t *mr,
                                    void (*mapper_fn)(thread_t *, mr_iovec_t *, mr_iovec_t **));

void 
map_reduce_set_mapper_input(map_reduce_t *mr, int index, mr_iovec_t *input);

void
map_reduce_set_reducer_fn (map_reduce_t *mr, 
                                     void (*reducer_fn)(thread_t *, mr_iovec_t **, int, mr_iovec_t *));

void
map_reduce_set_reducer_output_reader (map_reduce_t *mr,
                                    void (*reducer_output_reader)(mr_iovec_t *iovec));

void
map_reduce_start(map_reduce_t *mr);

bool
map_reduce_is_in_progress(map_reduce_t *mr);

#endif /* __THREAD_LIB__  */

/*
  Visit : www.csepracticals.com for more courses and projects
  Join Telegram Grp : telecsepracticals
*/

