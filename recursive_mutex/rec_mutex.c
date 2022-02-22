#include <stdio.h>
#include <assert.h>
#include "rec_mutex.h"

void
rec_mutex_init(rec_mutex_t *rec_mutex)  {

    rec_mutex->n = 0;
    rec_mutex->locking_thread = 0;
    pthread_cond_init(&rec_mutex->cv, NULL);
    pthread_mutex_init(&rec_mutex->state_mutex, NULL);
    rec_mutex->n_waited = 0;
}

void
rec_mutex_lock(rec_mutex_t *rec_mutex) {

    pthread_mutex_lock(&rec_mutex->state_mutex);

    /*case 1 : When rec mutex object is not already locked */
    if (rec_mutex->n == 0) {
        assert(rec_mutex->locking_thread == 0);
        rec_mutex->n = 1;
        rec_mutex->locking_thread = pthread_self();
        pthread_mutex_unlock(&rec_mutex->state_mutex);
        return;
    }

    /*case 2 :  When rec_mutex is locked by self already */
    if (rec_mutex->locking_thread == pthread_self()) {
        assert(rec_mutex->n);
        rec_mutex->n++;
        pthread_mutex_unlock(&rec_mutex->state_mutex);
        return;
    }

    /*case 3 : When this rec_mutex object is locked by some other thread*/
    while (rec_mutex->locking_thread &&
            rec_mutex->locking_thread != pthread_self()) {

        rec_mutex->n_waited++;
        pthread_cond_wait(&rec_mutex->cv, &rec_mutex->state_mutex);
        rec_mutex->n_waited--;
    }

    assert(rec_mutex->n == 0);
    assert(rec_mutex->locking_thread == 0);

    rec_mutex->n = 1;
    rec_mutex->locking_thread = pthread_self();
    pthread_mutex_unlock(&rec_mutex->state_mutex);
}

void
rec_mutex_unlock(rec_mutex_t *rec_mutex) {

    pthread_mutex_lock(&rec_mutex->state_mutex);
    
     /*case 1 : When rec mutex object is not already locked */
    if (rec_mutex->n == 0) {
        assert(rec_mutex->locking_thread == 0);
        assert(0);
    }

     /*case 2 :  When rec_mutex is locked by self already */
     if (rec_mutex->locking_thread == pthread_self()) {

         assert(rec_mutex->n);
         rec_mutex->n--;

         if (rec_mutex->n > 0) {
             pthread_mutex_unlock(&rec_mutex->state_mutex);
             return;
         }

        if (rec_mutex->n_waited) {
                pthread_cond_signal(&rec_mutex->cv);
        }

        rec_mutex->locking_thread = 0;
        pthread_mutex_unlock(&rec_mutex->state_mutex);
        return;
     }

     /*case 3 : When this rec_mutex object is locked by some other thread*/
     while (rec_mutex->locking_thread &&
            rec_mutex->locking_thread != pthread_self()) {

        assert(0);
    }
}


void
rec_mutex_destroy(rec_mutex_t *rec_mutex) {

    assert(!rec_mutex->n);
    assert(!rec_mutex->locking_thread);
    assert(!rec_mutex->n_waited);
    pthread_mutex_destroy(&rec_mutex->state_mutex);
    pthread_cond_destroy(&rec_mutex->cv);
}
