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
   
   /* Case 1 : When rec_mutex is not locked*/
   if (rec_mutex->n == 0) {
       assert(rec_mutex->locking_thread == 0);
       rec_mutex->n = 1;
       rec_mutex->locking_thread = pthread_self();
       pthread_mutex_unlock(&rec_mutex->state_mutex);
       return;
   }
   
   /* Case 2 : When rec_mutex is locked by self */
   if (rec_mutex->locking_thread == pthread_self()) {
       assert(rec_mutex->n);
       rec_mutex->n++;
       pthread_mutex_unlock(&rec_mutex->state_mutex);
       return;
   }

   /* case 3 : When rec_mutex is locked by some other thread. Use while instead
        of if to handle spurious wake ups */
   while (rec_mutex->locking_thread &&
              rec_mutex->locking_thread != pthread_self()) {

            rec_mutex->n_waited++;
            pthread_cond_wait(&rec_mutex->cv, &rec_mutex->state_mutex);
            rec_mutex->n_waited--;
   }

    /* Sanity check that lock has been really released */
    assert(rec_mutex->n == 0);
    assert(rec_mutex->locking_thread == 0);

    /* When other thread release the mutex, we are signalled, go ahead and mark
    the recursive mutex as locked by self */
    rec_mutex->n = 1;
    rec_mutex->locking_thread = pthread_self();
    pthread_mutex_unlock(&rec_mutex->state_mutex);
}

void
rec_mutex_unlock(rec_mutex_t *rec_mutex) {

    pthread_mutex_lock(&rec_mutex->state_mutex);

    /* Case 1 : When rec_mutex is not locked*/
    if (rec_mutex->n == 0) {
        assert(rec_mutex->locking_thread == 0);
        assert(0);
    }

    /* Case 2 : When rec_mutex is locked by self */
    if (rec_mutex->locking_thread == pthread_self()) {

        assert(rec_mutex->n);
        rec_mutex->n--;

        /* Current thread still continue to hold a lock on rec mutex. Do nothing*/
        if (rec_mutex->n > 0) {
            pthread_mutex_unlock(&rec_mutex->state_mutex);
            return;
        }

        /* Current thread has completely released the mutex, send signal if required so that other thread waiting for this mutex can resume */
        if (rec_mutex->n_waited) {
            pthread_cond_signal(&rec_mutex->cv);
        }

        /* Mark the rec mutex as released */
        rec_mutex->locking_thread = 0;
        pthread_mutex_unlock(&rec_mutex->state_mutex);
        return;
    }

    /* case 3 : When rec_mutex is locked by some other thread*/
    if (rec_mutex->locking_thread &&
          rec_mutex->locking_thread != pthread_self()) {

        /* Why would a current thread unlock the mutex when it do not even
        hold it */
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


#if 0
int
main(int argc, char **argv) {

    rec_mutex_t rec_mutex;
    rec_mutex_init(&rec_mutex);
    rec_mutex_lock(&rec_mutex);
    rec_mutex_lock(&rec_mutex);
    rec_mutex_lock(&rec_mutex);
    printf("rec_mutex->n = %d, locking thread = %lu, n_waited = %d\n", rec_mutex.n, rec_mutex.locking_thread, rec_mutex.n_waited);
    rec_mutex_unlock(&rec_mutex);
    rec_mutex_unlock(&rec_mutex);
    rec_mutex_unlock(&rec_mutex);
    printf("rec_mutex->n = %d, locking thread = %lu, somebody waiting = %d\n", rec_mutex.n, rec_mutex.locking_thread, rec_mutex.n_waited);
    return 0;
}
#endif