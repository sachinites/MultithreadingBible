#include <assert.h>
#include "rw_locks.h"

void
rw_lock_init (rw_lock_t *rw_lock) {

    pthread_mutex_init(&rw_lock->state_mutex, NULL);
    pthread_cond_init(&rw_lock->cv, NULL);
    pthread_cond_init(&rw_lock->cv_readers, NULL);
    pthread_cond_init(&rw_lock->cv_writers, NULL);
    rw_lock->n_reader_waiting = 0;
    rw_lock->n_writer_waiting = 0;
    rw_lock->is_locked_by_reader = false;
    rw_lock->is_locked_by_writer = false;
    rw_lock->n_max_readers = RW_LOCK_MAX_READER_THREADS_DEF;
    rw_lock->n_max_writers = RW_LOCK_MAX_WRITER_THREADS_DEF;
    rw_lock->biasedness = RW_LOCK_BIASEDNESS_DEF;
}

/* Assumes state mutex is already locked */
static void
rw_lock_block_reader_thread(rw_lock_t *rw_lock) {

    switch (rw_lock->biasedness) {
        case RW_LOCK_BIASEDNESS_NEUTRAL:
            rw_lock->n_reader_waiting++;
            pthread_cond_wait(&rw_lock->cv, &rw_lock->state_mutex);
            rw_lock->n_reader_waiting--;
            break;
        case RW_LOCK_BIASEDNESS_READERS:
            rw_lock->n_reader_waiting++;
            pthread_cond_wait(&rw_lock->cv_readers, &rw_lock->state_mutex);
            rw_lock->n_reader_waiting--;
            break;
        case RW_LOCK_BIASEDNESS_WRITERS:
            assert(0);
        default :
            assert(0);
    }
}

/* Assumes state mutex is already locked */
static void
rw_lock_block_writer_thread(rw_lock_t *rw_lock) {

    switch (rw_lock->biasedness) {
        case RW_LOCK_BIASEDNESS_NEUTRAL:
            rw_lock->n_writer_waiting++;
            pthread_cond_wait(&rw_lock->cv, &rw_lock->state_mutex);
            rw_lock->n_writer_waiting--;
            break;
        case RW_LOCK_BIASEDNESS_READERS:
            assert(0);
        case RW_LOCK_BIASEDNESS_WRITERS:
            rw_lock->n_writer_waiting++;
            pthread_cond_wait(&rw_lock->cv_writers, &rw_lock->state_mutex);
            rw_lock->n_writer_waiting--;
        default :
            assert(0);
    }
}

static void
rw_lock_signal(rw_lock_t *rw_lock, bool broadcast,
                                bool exit_thread_is_reader) {

    if (rw_lock->n_reader_waiting == 0 &&
        rw_lock->n_writer_waiting == 0) {
            
            return;
    }

    switch (rw_lock->biasedness) {
        case RW_LOCK_BIASEDNESS_NEUTRAL:
            pthread_cond_broadcast(&rw_lock->cv);
            break;
        case RW_LOCK_BIASEDNESS_READERS:
            if (rw_lock->n_reader_waiting) {
                 if (broadcast) pthread_cond_broadcast(&rw_lock->cv_readers);
                 else pthread_cond_signal(&rw_lock->cv_readers);
            }
            else {
                if (broadcast) pthread_cond_broadcast(&rw_lock->cv_writers);
                else pthread_cond_signal(&rw_lock->cv_readers);
            }
            break;
        case RW_LOCK_BIASEDNESS_WRITERS:
            if (rw_lock->n_writer_waiting) {
                 if (broadcast) pthread_cond_broadcast(&rw_lock->cv_writers);
                 else pthread_cond_signal(&rw_lock->cv_writers);
            }
            else {
                if (broadcast) pthread_cond_broadcast(&rw_lock->cv_readers);
                else pthread_cond_signal(&rw_lock->cv_readers);
            }
            break;
        case RW_LOCK_BIASEDNESS_OPPOSITE:
            if (exit_thread_is_reader)
            {
                if (rw_lock->n_writer_waiting)
                {
                    if (broadcast)
                        pthread_cond_broadcast(&rw_lock->cv_writers);
                    else
                        pthread_cond_signal(&rw_lock->cv_writers);
                }
                else
                {
                    if (broadcast)
                        pthread_cond_broadcast(&rw_lock->cv_readers);
                    else
                        pthread_cond_signal(&rw_lock->cv_readers);
                }
            }
            else
            {
                if (rw_lock->n_reader_waiting)
                {
                    if (broadcast)
                        pthread_cond_broadcast(&rw_lock->cv_readers);
                    else
                        pthread_cond_signal(&rw_lock->cv_readers);
                }
                else
                {
                    if (broadcast)
                        pthread_cond_broadcast(&rw_lock->cv_writers);
                    else
                        pthread_cond_signal(&rw_lock->cv_readers);
                }
            }
            break;
        default :
            assert(0);
    }
}

void
rw_lock_rd_lock (rw_lock_t *rw_lock) {

    pthread_mutex_lock(&rw_lock->state_mutex);

    while (rw_lock->is_locked_by_writer ||
               rw_lock->n_locks == rw_lock->n_max_readers) {

        rw_lock_block_reader_thread(rw_lock);
    }

    if (rw_lock->n_locks == 0) {
        /* First Reader thread Enter C.S */
        rw_lock->is_locked_by_reader = true;
    }
    rw_lock->n_locks++;
    pthread_mutex_unlock(&rw_lock->state_mutex);
}

void
rw_lock_wr_lock (rw_lock_t *rw_lock) {

    pthread_mutex_lock(&rw_lock->state_mutex);

    while (rw_lock->is_locked_by_reader ||
               rw_lock->n_locks == rw_lock->n_max_writers) {

        rw_lock_block_writer_thread(rw_lock);
    }

    if (rw_lock->n_locks == 0) {
        /* First Writer thread Enter C.S */
        rw_lock->is_locked_by_writer = true;
    }
    rw_lock->n_locks++;
    pthread_mutex_unlock(&rw_lock->state_mutex);
}


void
rw_lock_unlock (rw_lock_t *rw_lock) {

    pthread_mutex_lock(&rw_lock->state_mutex);

    assert(rw_lock->n_locks);

    rw_lock->n_locks--;

    if (rw_lock->is_locked_by_writer) {

        if (rw_lock->n_locks == 0) {

            /* Last Writer has moved out of C.S*/
            rw_lock->is_locked_by_writer = false;
            rw_lock_signal(rw_lock, true, false);
        }
        else {
            /* only one writer thread as moved out of C.S*/
            if (rw_lock->n_writer_waiting) {
                if (rw_lock->biasedness == RW_LOCK_BIASEDNESS_NEUTRAL) {
                     rw_lock_signal(rw_lock, true, false);
                }
                else {
                    pthread_cond_signal(&rw_lock->cv_writers);
                }
            }
        }
    }

    else if (rw_lock->is_locked_by_reader) {

        if (rw_lock->n_locks == 0) {

            /* Last reader has moved out of C.S*/
            rw_lock->is_locked_by_reader = false;
            rw_lock_signal(rw_lock, true, true);
        }
        else {
            /* only one reader thread has moved out of C.S*/
            if (rw_lock->n_reader_waiting) {
                if (rw_lock->biasedness == RW_LOCK_BIASEDNESS_NEUTRAL) {
                    rw_lock_signal(rw_lock, true, true);
                }
                else {
                    pthread_cond_signal(&rw_lock->cv_readers);
                }
            }
        }
    }
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
    pthread_cond_destroy(&rw_lock->cv_readers);
    pthread_cond_destroy(&rw_lock->cv_writers);
}

void
rw_lock_set_max_readers_writers(rw_lock_t *rw_lock,
                                                        uint16_t max_readers, uint16_t max_writers) {

    rw_lock->n_max_readers = max_readers;
    rw_lock->n_max_writers = max_writers;
}

void
rw_lock_set_biasedness(rw_lock_t *rw_lock, uint8_t biasedness) {

    rw_lock->biasedness = biasedness;
}