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
    rw_lock->writer_thread = 0;
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
        assert(!rw_lock->writer_thread);
        rw_lock->n_locks++;
        rw_lock->is_locked_by_reader = true;
        pthread_mutex_unlock(&rw_lock->state_mutex);
        return;
    }

    /* Case 2 : rw_lock is locked by reader(s) thread already ( could be same as this thread (recursive)) */
    if (rw_lock->is_locked_by_reader) {
        
        assert(rw_lock->is_locked_by_writer == false);
        assert(rw_lock->n_locks);
        assert(!rw_lock->writer_thread);
        rw_lock->n_locks++;
        pthread_mutex_unlock(&rw_lock->state_mutex);
        return;
    }

    /* Case 3 : rw_lock is locked by a writer thread*/
     while (rw_lock->is_locked_by_writer) {

          assert(rw_lock->n_locks);
          assert(rw_lock->is_locked_by_reader == false);
          assert(rw_lock->writer_thread);
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
     assert(!rw_lock->writer_thread);
     pthread_mutex_unlock(&rw_lock->state_mutex);
}

void
rw_lock_unlock (rw_lock_t *rw_lock) {

    pthread_mutex_lock(&rw_lock->state_mutex);

    /* case 1 : Attempt to unlock the unlocked lock */
    assert(rw_lock->n_locks);

    /* Case 2 : Writer thread unlocking the rw_lock */
    if (rw_lock->is_locked_by_writer) {

        /* Only the owner of the rw_lock must attempt to unlock the rw_lock*/
        assert(pthread_self() == rw_lock->writer_thread);
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
        rw_lock->writer_thread = 0;
        pthread_mutex_unlock(&rw_lock->state_mutex);
        return;
    }

    /* Case 3 : Reader Thread trying to unlock the rw_lock */
    /* Note : Case 3 cannot be moved before Case 2. I leave it to you
        for reasoning.. */

    /* There is minor design flaw in our implementation. Not actually flaw, but
    a room to mis-use the rw_locks which is a trade off for simplicity. Here Simplicity is
    that rw_lock implementation do not keep track of "who" all reader threads have
    obtain the read lock on rw_lock. Here is an example, how rw_lock implementation
    could be erroneously used by the developer, yet rw_lock implementation would not
    report a bug ( assert () ).
    Suppose, threads T1, T2 and T3 owns read locks on rw_lock. and Now, some
    Thread T4 which do not own any type of lock on rw_lock invoke rw_lock_unlock( )
    API. The API would still honor the unlock request, since our rw_lock do not keep
    track of who all "reader" threads owns the rw_lock and treat T4 as some reader thread
    tryint to unlock the rw_lock*/

    else if (rw_lock->is_locked_by_reader) {

         assert(rw_lock->is_locked_by_writer == false);
         assert(rw_lock->writer_thread == 0);

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
        assert(!rw_lock->writer_thread);
        rw_lock->n_locks++;
        rw_lock->is_locked_by_writer = true;
        rw_lock->writer_thread = pthread_self();
        pthread_mutex_unlock(&rw_lock->state_mutex);
        return;
    }

    /*case 2: if same thread tries to obtain write lock again,
    Implement recursive property */
    if (rw_lock->is_locked_by_writer &&
         rw_lock->writer_thread == pthread_self()) {

        assert(rw_lock->is_locked_by_reader == false);
        assert(rw_lock->n_locks);

        rw_lock->n_locks++;
        pthread_mutex_unlock(&rw_lock->state_mutex);
        return;
    }

    /* Case 3 : If some thread tries to obtain a lock on already
       locked mutex */
    while (rw_lock->is_locked_by_reader || 
               rw_lock->is_locked_by_writer ) {

        /* Sanity check */
        if (rw_lock->is_locked_by_reader) {
            assert(rw_lock->is_locked_by_writer == false);
            assert(rw_lock->n_locks);
            assert(rw_lock->writer_thread == 0);
        } else if (rw_lock->is_locked_by_writer) {
            assert(rw_lock->is_locked_by_reader == false);
            assert(rw_lock->n_locks);
            assert(rw_lock->writer_thread);
        }

        rw_lock->n_writer_waiting++;
        pthread_cond_wait(&rw_lock->cv, &rw_lock->state_mutex);
        rw_lock->n_writer_waiting--;
    }

     /* Lock is Available now */
      assert(rw_lock->is_locked_by_reader == false);
      assert(rw_lock->is_locked_by_writer == false);
      assert(rw_lock->n_locks == 0);
      assert(!rw_lock->writer_thread);
      rw_lock->is_locked_by_writer = true;
      rw_lock->n_locks++;
      rw_lock->writer_thread = pthread_self();
      pthread_mutex_unlock(&rw_lock->state_mutex);
}

void
rw_lock_destroy(rw_lock_t *rw_lock) {

    assert(rw_lock->n_locks == 0);
    assert(rw_lock->n_reader_waiting == 0);
    assert(rw_lock->n_writer_waiting == 0);
    assert(rw_lock->is_locked_by_writer == false);
    assert(rw_lock->is_locked_by_reader == false);
    assert(!rw_lock->writer_thread);
    pthread_mutex_destroy(&rw_lock->state_mutex);
    pthread_cond_destroy(&rw_lock->cv);
}