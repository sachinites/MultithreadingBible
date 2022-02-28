#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include "rw_locks.h"

static rw_lock_t rw_lock;
static int n_r =  0;
static int n_w = 0;
pthread_mutex_t state_check_mutex;

static void
bridge_enter_request_LR_vehicle(rw_lock_t *rw_lock) {

    rw_lock_rd_lock(rw_lock);
    pthread_mutex_lock(&state_check_mutex);
    n_r++;
    pthread_mutex_unlock(&state_check_mutex);
}

static void
bridge_enter_request_RL_vehicle(rw_lock_t *rw_lock) {

    rw_lock_wr_lock(rw_lock);
    pthread_mutex_lock(&state_check_mutex);
    n_w++;
    pthread_mutex_unlock(&state_check_mutex);
}

static void
bridge_leave_LR_vehicle(rw_lock_t *rw_lock) {

    pthread_mutex_lock(&state_check_mutex);
    n_r--;
    pthread_mutex_unlock(&state_check_mutex);
    rw_lock_unlock(rw_lock);
}

static void
bridge_leave_RL_vehicle(rw_lock_t *rw_lock) {

    pthread_mutex_lock(&state_check_mutex);
    n_w--;
    pthread_mutex_unlock(&state_check_mutex);
    rw_lock_unlock(rw_lock);
}

static void
bridge_snapshot(rw_lock_t *rw_lock) {

    pthread_mutex_lock(&state_check_mutex);
    assert(n_r >= 0 && n_w >= 0); /* Cannot be negative */

    if (n_r == 0 && n_w <= rw_lock->n_max_writers) {
        // valid condition
    }
    else if (n_r <= rw_lock->n_max_readers && n_w == 0) {
        // valid condition
    }
    else {
        printf ("n_r = %d, n_w = %d\n", n_r, n_w);
        assert(0);
    }

    printf ("n_r = %d, n_w = %d\n", n_r, n_w);
    pthread_mutex_unlock(&state_check_mutex);
}

void *
LR_thread_fn (void *arg) {

    while(1) {
        bridge_enter_request_LR_vehicle(&rw_lock);
        
        bridge_snapshot(&rw_lock);

        bridge_leave_LR_vehicle(&rw_lock);
    }
}

void *
RL_thread_fn (void *arg) {

    while(1) {
         bridge_enter_request_RL_vehicle(&rw_lock);
        
        bridge_snapshot(&rw_lock);

        bridge_leave_RL_vehicle(&rw_lock);
    }
}

int
main(int argc, char **argv) {

   static pthread_t th1, th2, th3, th4, th5, th6;
    rw_lock_init(&rw_lock);
    rw_lock_set_max_readers_writers(&rw_lock, 3, 3);
    pthread_mutex_init (&state_check_mutex, NULL);
    pthread_create(&th1, NULL, LR_thread_fn, NULL);
    pthread_create(&th2, NULL, LR_thread_fn, NULL);
    pthread_create(&th3, NULL, LR_thread_fn, NULL);
    pthread_create(&th4, NULL, RL_thread_fn, NULL);
    pthread_create(&th5, NULL, RL_thread_fn, NULL);
    pthread_create(&th6, NULL, RL_thread_fn, NULL);
    pthread_exit(0);
    return 0;
}