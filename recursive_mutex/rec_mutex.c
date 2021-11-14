#include <stdio.h>
#include <assert.h>
#include "rec_mutex.h"

void
rec_mutex_init(rec_mutex_t *rec_mutex)  {

    pthread_mutex_init(&MUTEX(rec_mutex), NULL);
    rec_mutex->locking_thread = NULL;
    rec_mutex->n = 0;
    pthread_mutex_init(&rec_mutex->rec_state_mutex, NULL);
}

void
rec_mutex_lock(rec_mutex_t *rec_mutex) {

    pthread_mutex_lock(&rec_mutex->rec_state_mutex);

    if (rec_mutex->n == 0) {
        assert(!rec_mutex->locking_thread);
        rec_mutex->n = 1;
        rec_mutex->locking_thread = pthread_self();
        pthread_mutex_lock(&MUTEX(rec_mutex));
        pthread_mutex_unlock(&rec_mutex->rec_state_mutex);
        return;
    }

    else if (rec_mutex->locking_thread == pthread_self()) {
        assert(rec_mutex->n);
        rec_mutex->n++;
        pthread_mutex_unlock(&rec_mutex->rec_state_mutex);
        return;
    }

    pthread_mutex_lock(&MUTEX(rec_mutex));
    rec_mutex->n++;
    rec_mutex->locking_thread = pthread_self();
    pthread_mutex_unlock(&rec_mutex->rec_state_mutex);
}

void
rec_mutex_unlock(rec_mutex_t *rec_mutex) {

    pthread_mutex_lock(&rec_mutex->rec_state_mutex);

    /* Trying to unlock an unlocked mutex OR trying to unlock the mutex
        which is locked by somebody else */
    if (rec_mutex->n == 0 ||
          rec_mutex->locking_thread != pthread_self()) {
        assert(0);
    }

    else if (rec_mutex->locking_thread == pthread_self()) {
        rec_mutex->n--;
        if (rec_mutex->n == 0) {
            rec_mutex->locking_thread = NULL;
            pthread_mutex_unlock(&MUTEX(rec_mutex));
        }
    }
    pthread_mutex_unlock(&rec_mutex->rec_state_mutex);
}


int
main(int argc, char **argv) {

    rec_mutex_t rec_mutex;
    rec_mutex_init(&rec_mutex);
    rec_mutex_lock(&rec_mutex);
    rec_mutex_lock(&rec_mutex);
    rec_mutex_lock(&rec_mutex);
    printf("rec_mutex->n = %d\n", rec_mutex.n);
    rec_mutex_unlock(&rec_mutex);
    rec_mutex_unlock(&rec_mutex);
    rec_mutex_unlock(&rec_mutex);
    printf("rec_mutex->n = %d\n", rec_mutex.n);
    return 0;
}