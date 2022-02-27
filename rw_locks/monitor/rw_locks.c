#include <assert.h>
#include "rw_locks.h"

void
rw_lock_init (rw_lock_t *rw_lock) {

    pthread_mutex_init(&rw_lock->state_mutex, NULL);
    pthread_cond_init(&rw_lock->cv, NULL);
    rw_lock->n_reader_waiting = 0;
    rw_lock->n_writer_waiting = 0;
    rw_lock->is_locked_by_reader = false;
    rw_lock->is_locked_by_writer = false;
}

void
rw_lock_rd_lock (rw_lock_t *rw_lock) {

    pthread_mutex_lock(&rw_lock->state_mutex);

    /* Case 1 : rw_lock is Unlocked */
    if (rw_lock->is_locked_by_reader == false &&
         rw_lock->is_locked_by_writer == false) {

        assert(rw_lock->n_locks == 0);
        assert(!rw_lock->is_locked_by_reader);
        assert(!rw_lock->is_locked_by_writer);
        rw_lock->n_locks++;
        rw_lock->is_locked_by_reader = true;
        pthread_mutex_unlock(&rw_lock->state_mutex);
        return;
    }

    /* Case 2 : rw_lock is locked by reader(s) thread already ( could be same as this thread (recursive)) */
    if (rw_lock->is_locked_by_reader) {
        
        assert(rw_lock->is_locked_by_writer == false);
        assert(rw_lock->n_locks);
        rw_lock->n_locks++;
        pthread_mutex_unlock(&rw_lock->state_mutex);
        return;
    }

    /* Case 3 : rw_lock is locked by a writer thread*/
     while (rw_lock->is_locked_by_writer) {

          assert(rw_lock->n_locks);
          assert(rw_lock->is_locked_by_reader == false);
          rw_lock->n_reader_waiting++;
          pthread_cond_wait(&rw_lock->cv, &rw_lock->state_mutex);
          rw_lock->n_reader_waiting--;
     }

    if (rw_lock->n_locks == 0) {
        /* First reader enter C.S */
        rw_lock->is_locked_by_reader = true;
    }
     rw_lock->n_locks++;
     assert (rw_lock->is_locked_by_writer == false);
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
             pthread_mutex_unlock(&rw_lock->state_mutex);
             return;
        }

        if (rw_lock->n_reader_waiting ||
            rw_lock->n_writer_waiting) {

            pthread_cond_broadcast(&rw_lock->cv);
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
             pthread_mutex_unlock(&rw_lock->state_mutex);
             return;
         }

         if (rw_lock->n_reader_waiting ||
             rw_lock->n_writer_waiting) {

             pthread_cond_broadcast(&rw_lock->cv);
         }

         rw_lock->is_locked_by_reader = false;
         pthread_mutex_unlock(&rw_lock->state_mutex);
         return;
    }

    assert(0);
}

void
rw_lock_wr_lock (rw_lock_t *rw_lock) {

    pthread_mutex_lock(&rw_lock->state_mutex);

    /* Case 1 : rw_lock is Unlocked */
    if (rw_lock->is_locked_by_reader == false &&
          rw_lock->is_locked_by_writer == false) {

        assert(rw_lock->n_locks == 0);
        assert(!rw_lock->is_locked_by_reader);
        assert(!rw_lock->is_locked_by_writer);
        rw_lock->n_locks++;
        rw_lock->is_locked_by_writer = true;
        pthread_mutex_unlock(&rw_lock->state_mutex);
        return;
    }

    /* Case 2 : rw_lock is locked by writer(s) thread already ( could be same as this thread (recursive)) */
    if (rw_lock->is_locked_by_writer) {
        
        assert(rw_lock->is_locked_by_reader == false);
        assert(rw_lock->n_locks);
        rw_lock->n_locks++;
        pthread_mutex_unlock(&rw_lock->state_mutex);
        return;
    }

    /* Case 3 : rw_lock is locked by a writer thread*/
     while (rw_lock->is_locked_by_reader) {

          assert(rw_lock->n_locks);
          assert(rw_lock->is_locked_by_writer == false);
          rw_lock->n_writer_waiting++;
          pthread_cond_wait(&rw_lock->cv, &rw_lock->state_mutex);
          rw_lock->n_writer_waiting--;
     }

    if (rw_lock->n_locks == 0) {
        /* First reader enter C.S */
        rw_lock->is_locked_by_writer = true;
    }
     rw_lock->n_locks++;
     assert (rw_lock->is_locked_by_reader == false);
     pthread_mutex_unlock(&rw_lock->state_mutex);
}

void
rw_lock_destroy(rw_lock_t *rw_lock) {

    assert(rw_lock->n_locks == 0);
    assert(rw_lock->n_reader_waiting == 0);
    assert(rw_lock->n_writer_waiting == 0);
    assert(rw_lock->is_locked_by_writer == false);
    assert(rw_lock->is_locked_by_reader == false);
    pthread_mutex_destroy(&rw_lock->state_mutex);
    pthread_cond_destroy(&rw_lock->cv);
}