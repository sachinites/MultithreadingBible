#include <assert.h>
#include "thread_pause.h"

void
thread_pause_struct_init (thread_pause_struct_t *thread_pause_struct) {

    pthread_mutex_init (&thread_pause_struct->state_mutex, NULL);
    pthread_cond_init (&thread_pause_struct->pause_cv, NULL);
    pthread_cond_init (&thread_pause_struct->notif_cv, NULL);
    thread_pause_struct->should_pause = false;
}

void
thread_pause(thread_pause_struct_t *thread_pause_struct) {

    pthread_mutex_lock (&thread_pause_struct->state_mutex);
    
    if (thread_pause_struct->should_pause) {
        pthread_mutex_unlock (&thread_pause_struct->state_mutex);
        return;
    }

    thread_pause_struct->should_pause = true;
    pthread_cond_wait (&thread_pause_struct->notif_cv,
        &thread_pause_struct->state_mutex);

    pthread_mutex_unlock(&thread_pause_struct->state_mutex);
}

void
thread_resume(thread_pause_struct_t *thread_pause_struct) {

     pthread_mutex_lock (&thread_pause_struct->state_mutex);
     pthread_cond_signal(&thread_pause_struct->pause_cv);
     pthread_mutex_unlock(&thread_pause_struct->state_mutex);
}

void
thread_test_and_pause(thread_pause_struct_t *thread_pause_struct) {

    pthread_mutex_lock (&thread_pause_struct->state_mutex);

    if (!thread_pause_struct->should_pause) {
        pthread_mutex_unlock(&thread_pause_struct->state_mutex);
        return;
    }

    pthread_cond_signal(&thread_pause_struct->notif_cv);

    while (thread_pause_struct->should_pause) {
        pthread_cond_wait (&thread_pause_struct->pause_cv,
            &thread_pause_struct->state_mutex);
    }

    pthread_mutex_unlock(&thread_pause_struct->state_mutex);
}

void
thread_destroy_pause_struct(thread_pause_struct_t *thread_pause_struct) {
    
    assert(!thread_pause_struct->should_pause);
    pthread_mutex_destroy(&thread_pause_struct->state_mutex);
    pthread_cond_destroy(&thread_pause_struct->notif_cv);
    pthread_cond_destroy(&thread_pause_struct->pause_cv);
    
}