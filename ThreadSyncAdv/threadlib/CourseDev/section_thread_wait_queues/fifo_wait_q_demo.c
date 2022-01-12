#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include <stdbool.h>
#include "gluethread/glthread.h"
#include "threadlib.h"

static bool barrier_enabled = true;
static pthread_mutex_t app_mutex;
static wait_queue_t wq;

static void
enable_barrier() {
    pthread_mutex_lock(&app_mutex);
    barrier_enabled = true;
    pthread_mutex_unlock(&app_mutex);
}

static bool
is_barrier_enabled(void *arg, pthread_mutex_t **mutex){
    
    if (mutex) {
        *mutex = &app_mutex;
        pthread_mutex_lock(*mutex);
    }

    if (barrier_enabled) {
        return true;
    }

    return false;
}


static void*
thread_fn(void *arg) {

    thread_t *th = (thread_t *)arg;

    wait_queue_test_and_wait(&wq,  
                                                 is_barrier_enabled, NULL);
    
    pthread_mutex_unlock(wq.appln_mutex);

    return NULL;
}


int
main(int argc, char **argv) {

    wait_queue_init(&wq, true);
    
    pthread_mutex_init(&app_mutex, NULL);

    thread_t *th = thread_create(0, "Th1");
    thread_run(th, thread_fn, (void *)th);
    th = thread_create(0, "TH2");
    thread_run(th, thread_fn, (void *)th);
    th = thread_create(0, "TH3");
    thread_run(th, thread_fn, (void *)th);
    th = thread_create(0, "TH4");
    thread_run(th, thread_fn, (void *)th);
    th = thread_create(0, "TH5");
    thread_run(th, thread_fn, (void *)th);

    sleep(4);
    enable_barrier();
    wait_queue_signal(&wq, false);
    sleep(1);
    wait_queue_signal(&wq, false);
    sleep(1);
    wait_queue_signal(&wq, false);
    sleep(1);
    wait_queue_signal(&wq, false);
    sleep(1);
    wait_queue_signal(&wq, false);
    pthread_exit(0);
    return 0;
}
