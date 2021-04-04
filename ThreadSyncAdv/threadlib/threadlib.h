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
#include <semaphore.h>
#include <stdbool.h>
#include "../gluethread/glthread.h"
#include "Fifo_Queue.h"

typedef enum{

    THREAD_READER,
    THREAD_WRITER,
    THREAD_ANY
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
	uint32_t flags;
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





/*
  Visit : www.csepracticals.com for more courses and projects
  Join Telegram Grp : telecsepracticals
*/





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
       Thread pool to which the thread need to rest in peace after
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





/*
  Visit : www.csepracticals.com for more courses and projects
  Join Telegram Grp : telecsepracticals
*/





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
  Visit : www.csepracticals.com for more courses and projects
  Join Telegram Grp : telecsepracticals
*/






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
  /* Unlock application mutex automatically, true by default */
  bool auto_unlock_appln_mutex;
} wait_queue_t;


void
wait_queue_init (wait_queue_t * wq, bool priority_flag,
				 int (*insert_cmp_fn)(void *, void *));

void
wait_queue_set_auto_unlock_appln_mutex(wait_queue_t *wq, bool state);

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





/*
  Visit : www.csepracticals.com for more courses and projects
  Join Telegram Grp : telecsepracticals
*/








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





/*
  Visit : www.csepracticals.com for more courses and projects
  Join Telegram Grp : telecsepracticals
*/






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
	glthread_t active_threads_in_cs;

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
	
	/*
	  Enable strict alternation, useful to implement strict
	  producer-consumer thread synch scenarios. This will work only
	  When  n_readers_max_limit = n_writers_max_limit = 1. By default
	  it is false.
	*/
	bool strict_alternation;
	thread_op_type_t who_accessed_cs_last;
	
	/* Flag set when monitor needs to be shut-down */
	bool shutdown;
	bool is_deleted;
} monitor_t;

monitor_t *
init_monitor(monitor_t *monitor,
			 char *resource_name,
			 uint16_t n_readers_max_limit,
			 uint16_t n_writers_max_limit,
			 int (*monitor_wq_comp_fn_cb)(void *, void *));

void
monitor_set_wq_auto_mutex_lock_behavior(
		monitor_t *monitor,
		thread_op_type_t thread_type,
		bool wq_auto_locking_state) ;

void
monitor_set_strict_alternation_behavior(monitor_t *monitor);

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

void
print_monitor_snapshot(monitor_t *monitor);

void
monitor_sanity_check(monitor_t *monitor);

void
monitor_shut_down(monitor_t *monitor);

/* Monitor Implementation Ends Here */















/* Assembly line implementation starts here */
typedef void *(*generic_fn_ptr)(void *arg);

typedef enum {

    ASL_NOT_STARTED,
    ASL_IN_PROGRESS,
    ASL_WAIT,
    ASL_STOPPED
} asl_state_t;

typedef struct asl_worker_ asl_worker_t;

typedef struct assembly_line_ {
	
	/* Name of the Assembly line */
	char asl_name[64];
	/* Size of the Assembly line, which is fied */
	uint32_t asl_size;
        /* In which state is assembly line */
        asl_state_t asl_state;
	/* Assembly Queue of size N */
	Fifo_Queue_t *asl_q;
	/* Wait-Queue, assembly line will wait for next
	 item push until all workers have finished operation i.e.
	 wait until n_workers_finshed_opn == Current_size of CQ
	*/
	wait_queue_t asl_wq;
	/*no of worker threads finished their operation */
	wait_queue_t asl_ready_wq;
	/* no of workers ready to perform operation */
	uint32_t n_workers_ready;
	/* For updating properties of assembly_line_t y several threads
	   in a mutual exclusive way*/
	pthread_mutex_t mutex;
	/* List of workder threads */
	glthread_t worker_threads_head;
	/* asl engine thread */
	thread_t *asl_engine_thread;
	/*finished fn*/
	void (*asl_process_finished_product)(void *arg);
        /* Array of worker fns to be assigned to worker threads */
        generic_fn_ptr *work_fns;
        /* Wait list for items to pushed into ASL */
        Fifo_Queue_t *wait_lst_fq;
        /* Cache the first worker thread, used to push the new
         * element in ASL Queue */
        struct asl_worker_ *T0;
        /* Cache the last worker thread, used to remove the ASL item
         * from ASL Queue eventually */
        struct asl_worker_ *Tn;
} assembly_line_t;

struct asl_worker_ {
	
	/* Worker thread */
	thread_t *worker_thread;
	/* Slot no on which this worker thread operating*/
	uint32_t curr_slot;
	/* back ptr to assembly line for convinience*/
	assembly_line_t *asl;
	/* Work to be done by this worker thread on the ASL element */
        generic_fn_ptr work;
	/* Glue to Queue up worker thread in asl->worker_threads_head list*/
	glthread_t worker_thread_glue;
        /* Worker thread is ready to service assembly line */
	bool initialized;
};
GLTHREAD_TO_STRUCT(worker_thread_glue_to_asl_worker_thread,
				  asl_worker_t, worker_thread_glue);
	
assembly_line_t *
assembly_line_get_new_assembly_line(char *asl_name, uint32_t size,
                                void (*print_finished_fn)(void *arg));

void
assembly_line_register_worker_fns(assembly_line_t *asl,
                                  uint32_t slot,
                                  generic_fn_ptr work_fn);

void
assembly_line_init_worker_threads (assembly_line_t *asl);

void
assembly_line_push_new_item(assembly_line_t *asl,
			    void *new_item);

/*
 * Ques : 
 * Joining the two Assembly lines end-to-end
 * Rcycling through an assembly line (sugarcane juice machine)
 */

/* Assembly line implementation Ends here */









#endif /* __THREAD_LIB__  */



/*
  Visit : www.csepracticals.com for more courses and projects
  Join Telegram Grp : telecsepracticals
*/











