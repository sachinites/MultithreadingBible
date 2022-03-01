#include <assert.h>
#include "rw_locks.h"

void
rw_lock_init (rw_lock_t *rw_lock) {

    pthread_mutex_init(&rw_lock->state_mutex, NULL);
    pthread_cond_init(&rw_lock->cv_reader, NULL);
    pthread_cond_init(&rw_lock->cv_writer, NULL);
    rw_lock->dont_allow_readers = false;
    rw_lock->dont_allow_writers = false;
    rw_lock->n_reader_waiting = 0;
    rw_lock->n_writer_waiting = 0;
    rw_lock->is_locked_by_reader = false;
    rw_lock->is_locked_by_writer = false;
    rw_lock->n_max_readers = RW_LOCK_MAX_READER_THREADS_DEF;
    rw_lock->n_max_writers = RW_LOCK_MAX_WRITER_THREADS_DEF;
}

void
rw_lock_rd_lock (rw_lock_t *rw_lock) {

    pthread_mutex_lock(&rw_lock->state_mutex);

    /* Case 1 : rw_lock is Unlocked */
    if (rw_lock->is_locked_by_reader == false &&
         rw_lock->is_locked_by_writer == false &&
         rw_lock->n_max_readers >= 1 &&
         rw_lock->dont_allow_readers == false) {

        assert(rw_lock->n_locks == 0);
        assert(!rw_lock->is_locked_by_reader);
        assert(!rw_lock->is_locked_by_writer);
        rw_lock->n_locks++;
        rw_lock->is_locked_by_reader = true;
        pthread_mutex_unlock(&rw_lock->state_mutex);
        return;
    }

    /* Case 2 and 3 : rw_lock is locked by a writer thread Or lock is exhausted*/
     while (rw_lock->is_locked_by_writer ||
                rw_lock->n_locks == rw_lock->n_max_readers ||
                 rw_lock->dont_allow_readers) {

          rw_lock->n_reader_waiting++;
          pthread_cond_wait(&rw_lock->cv_reader, &rw_lock->state_mutex);
          rw_lock->n_reader_waiting--;
     }

    if (rw_lock->n_locks == 0) {
        /* First reader enter C.S */
        rw_lock->is_locked_by_reader = true;
        rw_lock->dont_allow_writers = false;
    }
     rw_lock->n_locks++;
     assert (rw_lock->is_locked_by_writer == false);
     pthread_mutex_unlock(&rw_lock->state_mutex);
}

void
rw_lock_wr_lock (rw_lock_t *rw_lock) {

    pthread_mutex_lock(&rw_lock->state_mutex);

    /* Case 1 : rw_lock is Unlocked */
    if (rw_lock->is_locked_by_reader == false &&
         rw_lock->is_locked_by_writer == false &&
         rw_lock->n_max_writers >= 1 &&
         rw_lock->dont_allow_writers == false) {

        assert(rw_lock->n_locks == 0);
        assert(!rw_lock->is_locked_by_reader);
        assert(!rw_lock->is_locked_by_writer);
        rw_lock->n_locks++;
        rw_lock->is_locked_by_writer = true;
        pthread_mutex_unlock(&rw_lock->state_mutex);
        return;
    }

    /* Case 2 and 3 : rw_lock is locked by a reader thread Or lock is exhausted*/
     while (rw_lock->is_locked_by_reader ||
                rw_lock->n_locks == rw_lock->n_max_writers ||
                rw_lock->dont_allow_writers) {

          rw_lock->n_writer_waiting++;
          pthread_cond_wait(&rw_lock->cv_writer, &rw_lock->state_mutex);
          rw_lock->n_writer_waiting--;
     }

    if (rw_lock->n_locks == 0) {
        /* First writer enter C.S */
        rw_lock->is_locked_by_writer = true;
        rw_lock->dont_allow_readers = false;
    }
     rw_lock->n_locks++;
     assert (rw_lock->is_locked_by_reader == false);
     pthread_mutex_unlock(&rw_lock->state_mutex);
}

void
rw_lock_unlock (rw_lock_t *rw_lock) {

    pthread_mutex_lock(&rw_lock->state_mutex);

    /* case 1 : Attempt to unlock the unlocked lock */
    assert(rw_lock->n_locks);

    /* Case 2 : Writer thread unlocking the rw_lock */
    if (rw_lock->is_locked_by_writer) {

        assert(rw_lock->is_locked_by_reader == false);

        rw_lock->n_locks--;
        
        if (rw_lock->n_locks) {
            /* Replacement Property */
            if (rw_lock->n_writer_waiting) {
                pthread_cond_signal(&rw_lock->cv_writer);
            }
             pthread_mutex_unlock(&rw_lock->state_mutex);
             return;
        }

        if (rw_lock->n_reader_waiting ) {
            pthread_cond_broadcast(&rw_lock->cv_reader);
            rw_lock->dont_allow_writers = true;
            rw_lock->dont_allow_readers = false;
        }
        else if (rw_lock->n_writer_waiting) {
            pthread_cond_broadcast(&rw_lock->cv_writer);
        }

        rw_lock->is_locked_by_writer = false;
        pthread_mutex_unlock(&rw_lock->state_mutex);
        return;
    }

    /* Case 3 : Reader Thread trying to unlock the rw_lock */

    else if (rw_lock->is_locked_by_reader) {

         assert(rw_lock->is_locked_by_writer == false);

         rw_lock->n_locks--;

         if (rw_lock->n_locks) {
             /* Replacement Property */
            if (rw_lock->n_reader_waiting) {
                pthread_cond_signal(&rw_lock->cv_reader);
            }
             pthread_mutex_unlock(&rw_lock->state_mutex);
             return;
         }

         if (rw_lock->n_writer_waiting ) {
             pthread_cond_broadcast(&rw_lock->cv_writer);
             rw_lock->dont_allow_writers = false;
            rw_lock->dont_allow_readers = true;
         }
         else if (rw_lock->n_reader_waiting ) {
             pthread_cond_broadcast(&rw_lock->cv_reader);
         }

         rw_lock->is_locked_by_reader = false;
         pthread_mutex_unlock(&rw_lock->state_mutex);
         return;
    }

    assert(0);
}

void
rw_lock_destroy(rw_lock_t *rw_lock) {

    assert(rw_lock->n_locks == 0);
    assert(rw_lock->n_reader_waiting == 0);
    assert(rw_lock->n_writer_waiting == 0);
    assert(rw_lock->is_locked_by_writer == false);
    assert(rw_lock->is_locked_by_reader == false);
    pthread_mutex_destroy(&rw_lock->state_mutex);
    pthread_cond_destroy(&rw_lock->cv_reader);
    pthread_cond_destroy(&rw_lock->cv_writer);

}

void
rw_lock_set_max_readers_writers(rw_lock_t *rw_lock,
                                                        uint16_t max_readers, uint16_t max_writers) {

    rw_lock->n_max_readers = max_readers;
    rw_lock->n_max_writers = max_writers;
}