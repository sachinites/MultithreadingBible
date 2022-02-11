#ifndef __RW_LOCKS__
#define __RW_LOCKS__

#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct rwlock_ {

    /* A Mutex to manipulate/inspect the state of rwlock 
    in a mutually exclusive way */
    pthread_mutex_t state_mutex;
    /* A CV to block the the threads when the lock is not
    available */
    pthread_cond_t cv;
    /* Count of number of concurrent threads executing inside C.S. */
    uint16_t n_locks;
    /* No of reader threads waiting for the lock grant*/
    uint16_t n_reader_waiting;
    /* No of writer threads waiting for the lock grant*/
    uint16_t n_writer_waiting;
    /* Is locked currently occupied by Reader threads */
    bool is_locked_by_reader;
    /* Is locked currently occupied by a Writer thread */
    bool is_locked_by_writer;
    /* Thread handle of the writer thread currently holding the lock
    It is 0 if lock is not being held by writer thread */
    pthread_t writer_thread;
}rw_lock_t;

void
rw_lock_init (rw_lock_t *rw_lock);

void
rw_lock_rd_lock (rw_lock_t *rw_lock);

void
rw_lock_wr_lock (rw_lock_t *rw_lock);

void
rw_lock_unlock (rw_lock_t *rw_lock) ;

void
rw_lock_destroy(rw_lock_t *rw_lock);

#endif 