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
    glthread_t wait_glue;
} thread_t;
GLTHREAD_TO_STRUCT(wait_glue_to_thread,
        thread_t, wait_glue);

thread_t *
create_thread(thread_t *thread, char *name,
				thread_op_type_t thread_op);

void
run_thread(thread_t *thread, void *(*thread_fn)(void *), void *arg);

void
thread_lib_print_thread(thread_t *thread);











/* Thread Pool Begin */

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

/* Thread Pool End */











/* Event Pair Begin */

typedef struct event_pair_ {
	
	sem_t client_sem0;
	sem_t server_sem0;
	sem_t initial_sync_sem0;
} event_pair_t;

void
event_pair_server_init (event_pair_t *ep);

void
event_pair_client_init (event_pair_t *ep);

/* Blocks the calling thread */
void
event_pair_server_wait (event_pair_t *ep);

void
event_pair_client_wait (event_pair_t *ep);

void
event_pair_initial_sync_wait (event_pair_t *ep);

void
event_pair_initial_sync_signal (event_pair_t *ep);

/* Signal the other party */
void
event_pair_server_handoff (event_pair_t *ep);

void
event_pair_client_handoff (event_pair_t *ep);

void
event_pair_server_destroy (event_pair_t *ep);

void
event_pair_client_destroy (event_pair_t *ep);

/* Event Pair End */












/*
 wait queue Implementation starts here 
 */

/* 
   Fn pointer definition of wait_queue block fn. The second parameter
   locked_appln_mutex is an output param
*/
typedef bool (*wait_queue_block_fn)
  (void *appln_arg, pthread_mutex_t ** locked_appln_mutex);

typedef struct wait_queue_
{
  bool priority_flag;
 /* No of threads waiting in a wait-queue*/
  uint32_t thread_wait_count;
 /* CV on which multiple threads in wait-queue are blocked */
  pthread_cond_t cv;
 /* Application owned Mutex cached by wait-queue */ 
  pthread_mutex_t *appln_mutex;
  /* List of threads blocked on this wait Queue */
  glthread_t priority_wait_queue_head;  
  /* Comparison fn to insert the threads in the PQ */
  int (*insert_cmp_fn)(void *, void *);
} wait_queue_t;


void
wait_queue_init (wait_queue_t * wq, bool priority_flag,
				 int (*insert_cmp_fn)(void *, void *));

thread_t *
wait_queue_test_and_wait (wait_queue_t *wq,
			  wait_queue_block_fn wait_queue_block_fn_cb,
			  void *arg,
			  thread_t *thread);

void
wait_queue_signal (wait_queue_t *wq, bool lock_mutex);

void
wait_queue_broadcast (wait_queue_t *wq, bool lock_mutex);

void
wait_queue_print(wait_queue_t *wq);

/* wait queue Implementation ends here */














/* Thread Barrier Implementation Starts here */

typedef struct th_barrier_ {

 	uint32_t max_count;
	uint32_t curr_wait_count;
	pthread_cond_t cv;
	pthread_mutex_t mutex;
	bool is_ready_again;
	pthread_cond_t busy_cv;
} th_barrier_t;

void
thread_barrier_print(th_barrier_t *th_barrier);

void
thread_barrier_init ( th_barrier_t *barrier, uint32_t count);

void
thread_barrier_signal_all ( th_barrier_t *barrier);
                     
void
thread_barrier_barricade ( th_barrier_t *barrier);


/* Thread Barrier Implementation Ends here */












/* Monitor Implementation Starts Here */

typedef enum {

	MON_RES_AVAILABLE,
	MON_RES_BUSY_BY_READER,
	MON_RES_BUSY_BY_WRITER,
} resource_status_t;

typedef struct monitor_{

	/*  Name of the resource which is protected by this Monitor */
	char name[32];	

	/* Threads (clients) will talk to monitor in a Mutual Exclusion Way */
	pthread_mutex_t monitor_talk_mutex;

	/* List of writer threads  waiting on a resource*/
	wait_queue_t writer_thread_wait_q;

	/* List of Reader threads Waiting on a resource */
	wait_queue_t reader_thread_wait_q;

	/* List of Threads using the resource currently,
     * Multiple threads if Multiple Readers are accessing,
     * Only one thread in list of it is a Writer thread*/
	glthread_t resource_using_threads_q;

	/* Status of the resource */
	resource_status_t resource_status;
	
	/* No of Concurrent readers allowed to access the resource */
	uint16_t n_readers_max_limit;
	
	/* No of Concurrent writers allowed to access the resource */
	uint16_t n_writers_max_limit;
	
	/* Current no of Concurrent Readers/Writers accessing the resource */
	uint16_t n_curr_readers;
	uint16_t n_curr_writers;
	
	/*Stats*/
	uint16_t switch_from_readers_to_writers;
	uint16_t switch_from_writers_to_readers;
} monitor_t;

monitor_t *
init_monitor(monitor_t *monitor,
			 char *resource_name,
			 uint16_t n_readers_max_limit,
			 uint16_t n_writers_max_limit,
			 int (*monitor_wq_comp_fn_cb)(void *, void *));

/* fn used by the client thread to request read/write access
 * on a resource. Fn returns if permission is granted,
 * else the fn is blocked and stay blocked until request
 * is granted to the calling thread*/
void
monitor_request_access_permission (
	monitor_t *monitor,
	thread_t *requester_thread);

/* fn used by the client thread to tell the monitor that
 * client is done with the resource. Thid fn do not blocks*/
void
monitor_inform_resource_released (
	monitor_t *monitor,
	thread_t *requester_thread);

/* Monitor Implementation Ends Here */

#endif /* __THREAD_LIB__  */















