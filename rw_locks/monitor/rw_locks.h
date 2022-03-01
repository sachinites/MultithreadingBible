#ifndef __RW_LOCKS__
#define __RW_LOCKS__

#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>

#define RW_LOCK_MAX_READER_THREADS_DEF  0xFFFF
#define RW_LOCK_MAX_WRITER_THREADS_DEF  1

typedef struct rwlock_ {

    /* A Mutex to manipulate/inspect the state of rwlock 
    in a mutually exclusive way */
    pthread_mutex_t state_mutex;
    /* A CV to block the reader threads when the lock is not  available */
    pthread_cond_t cv_reader;
     /* A CV to block the writer threads when the lock is not  available */
    pthread_cond_t cv_writer;
    /* A flag used to block make Reader threads ineligible for entering C.S */
    bool dont_allow_readers;
     /* A flag used to block make Writer threads ineligible for entering C.S */
    bool dont_allow_writers;
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
     /* Max no of Reader threads allowed to enter C.S */
    uint16_t n_max_readers;
    /* Max no of Writer threads allowed to enter C.S */
    uint16_t n_max_writers;
}rw_lock_t;

void
rw_lock_init (rw_lock_t *rw_lock);

void
rw_lock_set_max_readers_writers(rw_lock_t *rw_lock,
                                                        uint16_t max_readers,
                                                        uint16_t max_writers);

void
rw_lock_rd_lock (rw_lock_t *rw_lock);

void
rw_lock_wr_lock (rw_lock_t *rw_lock);

void
rw_lock_unlock (rw_lock_t *rw_lock) ;

void
rw_lock_destroy(rw_lock_t *rw_lock);

#endif 