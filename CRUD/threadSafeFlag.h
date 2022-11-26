#ifndef __THREAD_SAFE_FLAG__
#define __THREAD_SAFE_FLAG__

#include <pthread.h>
#include <stdint.h>

typedef struct tflag_ {

    uint32_t flags;
    pthread_spinlock_t spinlock;
} tflag_t;

static inline void
tflag_init (tflag_t *tflag) {

    tflag->flags = 0;
    pthread_spin_init(&tflag->spinlock, PTHREAD_PROCESS_PRIVATE);
}

static inline void
tflag_deinit (tflag_t *tflag) {

    tflag->flags = 0;
    pthread_spin_destroy(&tflag->spinlock);
}

static inline void 
tflag_set (tflag_t *tflag, uint32_t flag_bit) {

    pthread_spin_lock(&tflag->spinlock);
    tflag->flags |= flag_bit;
    pthread_spin_unlock(&tflag->spinlock);
}

static inline void 
tflag_unset (tflag_t *tflag, uint32_t flag_bit) {

    pthread_spin_lock(&tflag->spinlock);
    tflag->flags &= ~flag_bit;
    pthread_spin_unlock(&tflag->spinlock);
}

static inline void 
tflag_print (tflag_t *tflag, void (*print_fn)(tflag_t *)) {

    pthread_spin_lock(&tflag->spinlock);
    print_fn(tflag);
    pthread_spin_unlock(&tflag->spinlock);
}

static inline bool
tflag_check_state(tflag_t *tflag, bool (*state_check_fn)(tflag_t *)) {

    bool rc;
    pthread_spin_lock(&tflag->spinlock);
    rc = state_check_fn(tflag);
    pthread_spin_unlock(&tflag->spinlock);
    return rc;
}

#endif