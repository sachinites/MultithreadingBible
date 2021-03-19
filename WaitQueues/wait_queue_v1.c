#include <stdio.h>
#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>

typedef bool (*wait_queue_block_fn)
  (void *appln_arg, pthread_mutex_t ** locked_appln_mutex);

typedef struct wait_queue_
{

  uint32_t thread_wait_count;
  pthread_cond_t cv;
  pthread_mutex_t *appln_mutex;
} wait_queue_t;

void
wait_queue_init (wait_queue_t * wq)
{

  wq->thread_wait_count = 0;
  pthread_cond_init (&wq->cv, NULL);
  wq->appln_mutex = NULL;
}

void
wait_queue_re_init (wait_queue_t * wq)
{

  assert (wq->thread_wait_count == 0);
  wq->appln_mutex = NULL;
}

void
wait_queue_test_and_wait (wait_queue_t * wq,
			  wait_queue_block_fn wait_queue_block_fn_cb,
			  void *arg)
{

  bool should_block;
  pthread_mutex_t *locked_appln_mutex = NULL;

  should_block = wait_queue_block_fn_cb (arg, &locked_appln_mutex);

  assert (locked_appln_mutex);

  if (!wq->appln_mutex)
    {
      wq->appln_mutex = locked_appln_mutex;
    }
  else
    {
      assert (wq->appln_mutex == locked_appln_mutex);
    }

  while (should_block)
    {
      wq->thread_wait_count++;
      pthread_cond_wait (&wq->cv, wq->appln_mutex);
      wq->thread_wait_count--;
      should_block = wait_queue_block_fn_cb (arg, NULL);
    }

  if (wq->thread_wait_count == 0) {
      wq->appln_mutex = NULL;
  }
  
  pthread_mutex_unlock (locked_appln_mutex);
}

void
wait_queue_signal (wait_queue_t * wq)
{
  if (!wq->appln_mutex) return;
  pthread_mutex_lock(wq->appln_mutex);
  if (!wq->thread_wait_count) {
      pthread_mutex_unlock(wq->appln_mutex);
      return;
  }
  pthread_cond_signal (&wq->cv);
  pthread_mutex_unlock(wq->appln_mutex);
}

void
wait_queue_broadcast (wait_queue_t * wq)
{
  if (!wq->appln_mutex) return;
  pthread_mutex_lock(wq->appln_mutex);
  if (!wq->thread_wait_count) {
      pthread_mutex_unlock(wq->appln_mutex);
      return;
  }
  pthread_cond_broadcast (&wq->cv);
  pthread_mutex_unlock(wq->appln_mutex);
}

void
wait_queue_print(wait_queue_t *wq) {
    
    printf("wq->thread_wait_count = %u\n", wq->thread_wait_count);
    printf("wq->appl_result = %p\n", wq->appln_mutex);
}



/* Application fn to test wait-Queue Condition */

static wait_queue_t wq;

static bool appl_result = true;

bool
test_appln_condition_in_mutual_exclusive_way (void *app_arg,
					      pthread_mutex_t ** mutex)
{

  static pthread_mutex_t app_mutex;
  static bool initialized = false;

  if (!initialized) {
    pthread_mutex_init (&app_mutex, NULL);
    initialized = true;
  }
  
  *mutex = &app_mutex;
  return appl_result;
}

bool
test_appln_condition_without_lock (void *app_arg)
{

  return appl_result;
}

bool
appln_cond_test_fn (void *app_arg, pthread_mutex_t ** mutex)
{

  if (mutex)
    {
      return test_appln_condition_in_mutual_exclusive_way (app_arg, mutex);
    }
  return test_appln_condition_without_lock (app_arg);
}

void *
thread_fn_callback (void *arg)
{
  printf ("Thread %s invoking wait_queue_test_and_wait( ) \n", (char *) arg);
  wait_queue_test_and_wait (&wq, appln_cond_test_fn, NULL);
  pthread_exit(NULL);
  return NULL;
}

static pthread_t pthreads[3];

int
main (int argc, char **argv)
{
  wait_queue_init (&wq);

  const char *th1 = "th1";
  pthread_create (&pthreads[0], NULL, thread_fn_callback, (void *) th1);

  const char *th2 = "th2";
  pthread_create (&pthreads[1], NULL, thread_fn_callback, (void *) th2);

  const char *th3 = "th3";
  pthread_create (&pthreads[2], NULL, thread_fn_callback, (void *) th3);
  
  sleep(3);
  wait_queue_print(&wq);
  
  appl_result = false;
  wait_queue_signal(&wq);
  sleep(1);
  wait_queue_print(&wq);
  wait_queue_signal(&wq);
  sleep(1);
  wait_queue_print(&wq);
  wait_queue_signal(&wq);
  sleep(1);
  wait_queue_print(&wq);

  pthread_join (pthreads[0], NULL);
  printf("Thread %s joined\n", (char *)th1);
  wait_queue_print(&wq);
  
  pthread_join (pthreads[1], NULL);
  printf("Thread %s joined\n", (char *)th2);
  wait_queue_print(&wq);
  
  pthread_join (pthreads[2], NULL);
  printf("Thread %s joined\n", (char *)th3);
  wait_queue_print(&wq);
  
  return 0;
}
