#include <assert.h>
#include "rw_locks.h"

void
rw_lock_init (rw_lock_t *rw_lock) {

    rw_lock->lock_status = unlock_t;
    pthread_mutex_init(&rw_lock->mutex_lock_status, NULL);
    pthread_cond_init(&rw_lock->cv, NULL);
    rw_lock->n = 0;
}

void
rw_lock_rd_lock (rw_lock_t *rw_lock) {

    pthread_mutex_lock(&rw_lock->mutex_lock_status);

    if (rw_lock->lock_status == unlock_t) {
        rw_lock->lock_status = read_lock_t;
        rw_lock->n = 1;
        pthread_mutex_unlock(&rw_lock->mutex_lock_status);
        return;
    }

    if (rw_lock->lock_status == read_lock_t) {
        rw_lock->n++;
        pthread_mutex_unlock(&rw_lock->mutex_lock_status);
        return;
    }

    if (rw_lock->lock_status == write_lock_t) {

        while (rw_lock->lock_status == write_lock_t) {

            pthread_cond_wait(&rw_lock->cv, &rw_lock->mutex_lock_status);
        }
        rw_lock->n++;
        pthread_mutex_unlock(&rw_lock->mutex_lock_status);
        return;
    }
}

void
rw_lock_rd_unlock (rw_lock_t *rw_lock) {

    pthread_mutex_lock(&rw_lock->mutex_lock_status);

    if (rw_lock->lock_status == unlock_t) {
        pthread_mutex_unlock(&rw_lock->mutex_lock_status);
        return;
    }

    if (rw_lock->lock_status == write_lock_t) {
        assert(0);
    }

    /* Read locked */
    assert( rw_lock->n);
    rw_lock->n--;

     if (rw_lock->n == 0) {
         rw_lock->lock_status = unlock_t;
         pthread_cond_signal(&rw_lock->cv);
     }

     pthread_mutex_unlock(&rw_lock->mutex_lock_status);
}

void
rw_lock_wr_lock (rw_lock_t *rw_lock) {

pthread_mutex_lock(&rw_lock->mutex_lock_status);

    if (rw_lock->lock_status == unlock_t) {
        rw_lock->lock_status = write_lock_t;
        rw_lock->n = 1;
        pthread_mutex_unlock(&rw_lock->mutex_lock_status);
        return;
    }

    if (rw_lock->lock_status == write_lock_t ||
        rw_lock->lock_status == read_lock_t) {

        while (rw_lock->lock_status == write_lock_t ||
               rw_lock->lock_status == read_lock_t) {

            pthread_cond_wait(&rw_lock->cv, &rw_lock->mutex_lock_status);
        }
        rw_lock->n++;
        pthread_mutex_unlock(&rw_lock->mutex_lock_status);
        return;
    }
}

void
rw_lock_wr_unlock (rw_lock_t *rw_lock)  {

    pthread_mutex_lock(&rw_lock->mutex_lock_status);

    if (rw_lock->lock_status == unlock_t) {
        pthread_mutex_unlock(&rw_lock->mutex_lock_status);
        return;
    }

    if (rw_lock->lock_status == read_lock_t) {
        assert(0);
    }

    /* write locked */
    assert( rw_lock->n);
    rw_lock->n--;

     if (rw_lock->n == 0) {
         rw_lock->lock_status = unlock_t;
         pthread_cond_signal(&rw_lock->cv);
     }

     pthread_mutex_unlock(&rw_lock->mutex_lock_status);
}