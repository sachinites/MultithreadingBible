#include "refcount.h"

void
ref_count_init (ref_count_t *ref_count) {

    ref_count->ref_count = 0;
    pthread_spin_init(&ref_count->spinlock, PTHREAD_PROCESS_PRIVATE);
}

void
ref_count_inc (ref_count_t *ref_count) {

    pthread_spin_lock(&ref_count->spinlock);
    ref_count++;
    pthread_spin_unlock(&ref_count->spinlock);
}

bool
ref_count_dec (ref_count_t *ref_count) {

    bool rc;
    pthread_spin_lock(&ref_count->spinlock);
    ref_count--;
    rc = (ref_count == 0) ? true : false; 
    pthread_spin_unlock(&ref_count->spinlock);
    return rc;
}

void
ref_count_destroy (ref_count_t *ref_count) {

    assert(ref_count == 0);
    pthread_spin_destroy(&ref_count->spinlock);
}