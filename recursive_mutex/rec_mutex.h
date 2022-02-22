#ifndef __REC_MUTEX__
#define __REC_MUTEX__

#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>


typedef struct rec_mutex_ {

    /* No of self-locks taken */
    uint16_t n;
    /* pthread id of the thread which owns this mutex*/
    pthread_t locking_thread;
    /* A CV to make the threads block */
    pthread_cond_t cv;
    /* A Mutex to manupulate the state of this structure
        in a mutually exclusive way */
    pthread_mutex_t state_mutex;
    /* /* No of threads waiting for this Mutex lock Grant */
    uint16_t n_waited;
} rec_mutex_t;

void
rec_mutex_init(rec_mutex_t *rec_mutex) ;

void
rec_mutex_lock(rec_mutex_t *rec_mutex);

void
rec_mutex_unlock(rec_mutex_t *rec_mutex);

void
rec_mutex_destroy(rec_mutex_t *rec_mutex);

#endif
