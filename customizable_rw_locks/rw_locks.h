#ifndef __RW_LOCKS__
#define __RW_LOCKS__

#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>

#define RW_LOCK_MAX_READER_THREADS_DEF  0xFFFF
#define RW_LOCK_MAX_WRITER_THREADS_DEF  1
#define RW_LOCK_BIASEDNESS_NEUTRAL (0)
#define RW_LOCK_BIASEDNESS_READERS (1)
#define RW_LOCK_BIASEDNESS_WRITERS  (2)
#define RW_LOCK_BIASEDNESS_OPPOSITE (3)
#define RW_LOCK_BIASEDNESS_DEF  RW_LOCK_BIASEDNESS_NEUTRAL

typedef struct rwlock_ {

    /* A Mutex to manipulate/inspect the state of rwlock 
    in a mutually exclusive way */
    pthread_mutex_t state_mutex;
    /* A CV to block the the threads when the lock is not
    available */
    pthread_cond_t cv;
    /* A CV to block only reader threads when BIASEDNESS_READER is enabled */
    pthread_cond_t cv_readers;
    /* A CV to block only writer threads when BIASEDNESS_WRITER is enabled */
    pthread_cond_t cv_writers;
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
    /* Biasedness property */
    uint8_t biasedness;
}rw_lock_t;

void
rw_lock_init (rw_lock_t *rw_lock);

void
rw_lock_set_max_readers_writers(rw_lock_t *rw_lock,
                                                        uint16_t max_readers, uint16_t max_writers);

void
rw_lock_set_biasedness(rw_lock_t *rw_lock, uint8_t biasedness);

void
rw_lock_rd_lock (rw_lock_t *rw_lock);

void
rw_lock_wr_lock (rw_lock_t *rw_lock);

void
rw_lock_unlock (rw_lock_t *rw_lock) ;

void
rw_lock_destroy(rw_lock_t *rw_lock);

#endif 