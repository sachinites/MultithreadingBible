#ifndef __TH_PAUSE__
#define __TH_PAUSE__

#include <stdbool.h>
#include <pthread.h>

typedef struct thread_pause_struct_ {

    pthread_mutex_t state_mutex;
    pthread_cond_t pause_cv;
    bool should_pause;
    pthread_cond_t notif_cv; 
} thread_pause_struct_t;

void
thread_pause_struct_init (thread_pause_struct_t *thread_pause_struct);

void
thread_pause(thread_pause_struct_t *thread_pause_struct);

void
thread_resume(thread_pause_struct_t *thread_pause_struct);

void
thread_test_and_pause(thread_pause_struct_t *thread_pause_struct);

void
thread_destroy_pause_struct(thread_pause_struct_t *thread_pause_struct);

#endif 