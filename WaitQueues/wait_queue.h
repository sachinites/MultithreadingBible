
#ifndef __WAIT_QUEUE__
#define __WAIT_QUEUE__

#include <pthread.h>
#include <stdint.h>

/* 
   Fn pointer definition of wait_queue block fn. The second parameter
   locked_appln_mutex is an output param
*/
typedef bool (*wait_queue_block_fn)
  (void *appln_arg, pthread_mutex_t ** locked_appln_mutex);

typedef struct wait_queue_
{
 /* No of threads waiting in a wait-queue*/
  uint32_t thread_wait_count;
 /* CV on which multiple threads in wait-queue are blocked */
  pthread_cond_t cv;
 /* Application owned Mutex cached by wait-queue */ 
  pthread_mutex_t *appln_mutex;
} wait_queue_t;


void
wait_queue_init (wait_queue_t * wq);

void
wait_queue_re_init (wait_queue_t * wq);

void
wait_queue_test_and_wait (wait_queue_t * wq,
			  wait_queue_block_fn wait_queue_block_fn_cb,
			  void *arg);

void
wait_queue_signal (wait_queue_t * wq);

void
wait_queue_broadcast (wait_queue_t * wq);

void
wait_queue_print(wait_queue_t *wq);

#endif