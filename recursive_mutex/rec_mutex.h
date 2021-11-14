#ifndef __REC_MUTEX__
#define __REC_MUTEX__

#include <pthread.h>

typedef struct rec_mutex_ {

    /* Actual underlying mutex */
    pthread_mutex_t mutex;
    /* No of times it has been locked */
    int n;
    pthread_t locking_thread;
    pthread_mutex_t rec_state_mutex;
} rec_mutex_t;

/* Note is this recursive mutex is used along with CV, then use below macro
to get the actual hidden mutex to be paired up with CV */
#define MUTEX(rec_mutex_ptr)    ((rec_mutex_ptr)->mutex)

void
rec_mutex_init(rec_mutex_t *rec_mutex) ;

void
rec_mutex_lock(rec_mutex_t *rec_mutex);

void
rec_mutex_unlock(rec_mutex_t *rec_mutex);

#endif